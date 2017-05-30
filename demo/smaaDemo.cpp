/*
Copyright (c) 2015 Alternative Games Ltd / Turo Lamminen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <vector>

#include <imgui.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tclap/CmdLine.h>

#include "Renderer.h"
#include "Utils.h"

#include "AreaTex.h"
#include "SearchTex.h"


union Color {
	uint32_t val;
	struct {
		uint8_t r, g, b, a;
	};
};


static const Color white = { 0xFFFFFFFF };


class SMAADemo;


namespace AAMethod {

enum AAMethod {
	  FXAA
	, SMAA
	, LAST = SMAA
};


const char *name(AAMethod m) {
	switch (m) {
	case FXAA:
		return "FXAA";
		break;

	case SMAA:
		return "SMAA";
		break;
	}

	__builtin_unreachable();
}


}  // namespace AAMethod


static const char *smaaDebugModeStr(unsigned int mode) {
	switch (mode) {
	case 0:
		return "none";

	case 1:
		return "edges";

	case 2:
		return "blend";
	}

	__builtin_unreachable();
}


// *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
// Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)

typedef struct { uint64_t state;  uint64_t inc; } pcg32_random_t;

static uint32_t pcg32_random_r(pcg32_random_t* rng)
{
    uint64_t oldstate = rng->state;
    // Advance internal state
    rng->state = oldstate * 6364136223846793005ULL + (rng->inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}


class RandomGen {
	pcg32_random_t rng;

	RandomGen(const RandomGen &) = delete;
	RandomGen &operator=(const RandomGen &) = delete;
	RandomGen(RandomGen &&) = delete;
	RandomGen &operator=(RandomGen &&) = delete;

public:

	explicit RandomGen(uint64_t seed)
	{
		rng.state = seed;
		rng.inc = 1;
		// spin it once for proper initialization
		pcg32_random_r(&rng);
	}


	float randFloat() {
		uint32_t u = randU32();
		// because 24 bits mantissa
		u &= 0x00FFFFFFU;
		return float(u) / 0x00FFFFFFU;
	}


	uint32_t randU32() {
		return pcg32_random_r(&rng);
	}
};


static const char *fxaaQualityLevels[] =
{ "10", "15", "20", "29", "39" };


static const unsigned int maxFXAAQuality = sizeof(fxaaQualityLevels) / sizeof(fxaaQualityLevels[0]);


static const char *smaaQualityLevels[] =
{ "LOW", "MEDIUM", "HIGH", "ULTRA" };


static const unsigned int maxSMAAQuality = sizeof(smaaQualityLevels) / sizeof(smaaQualityLevels[0]);


namespace RenderTargets {

	enum RenderTargets {
		  MainColor
		, MainDepth
		, Edges
		, BlendWeights
		, FinalRender
		, Count
	};

}  // namespace RenderTargets


namespace Framebuffers {

	enum Framebuffers {
		  MainRender
		, Edges
		, BlendWeights
		, FinalRender
		, Count
	};

}  // namespace Framebuffers


class SMAADemo {
	unsigned int windowWidth, windowHeight;
	bool vsync;
	bool fullscreen;
	bool recreateSwapchain;

	Renderer renderer;
	bool glDebug;

	PipelineHandle cubePipeline;
	PipelineHandle imagePipeline;
	PipelineHandle guiPipeline;

	BufferHandle cubeVBO;
	BufferHandle cubeIBO;

	SamplerHandle linearSampler;
	SamplerHandle nearestSampler;

	unsigned int cubePower;

	std::array<RenderTargetHandle, RenderTargets::Count> rendertargets;
	std::array<FramebufferHandle, Framebuffers::Count> fbos;

	bool antialiasing;
	AAMethod::AAMethod aaMethod;

	std::array<PipelineHandle, maxFXAAQuality> fxaaPipelines;

	std::array<PipelineHandle, maxSMAAQuality> smaaEdgePipelines;
	std::array<PipelineHandle, maxSMAAQuality> smaaBlendWeightPipelines;
	std::array<PipelineHandle, maxSMAAQuality> smaaNeighborPipelines;
	TextureHandle areaTex;
	TextureHandle searchTex;

	TextureHandle imguiFontsTex;

	bool rotateCamera;
	float cameraRotation;
	uint64_t lastTime;
	uint64_t freq;
	uint64_t rotationTime;
	unsigned int debugMode;
	unsigned int colorMode;
	bool rightShift, leftShift;
	RandomGen random;
	unsigned int fxaaQuality;
	unsigned int smaaQuality;
	bool keepGoing;
	// 0 for cubes
	// 1.. for images
	unsigned int activeScene;


	struct Image {
		std::string filename;
		TextureHandle tex;
	};

	std::vector<Image> images;

	std::vector<ShaderDefines::Cube> cubes;

	SMAADemo(const SMAADemo &) = delete;
	SMAADemo &operator=(const SMAADemo &) = delete;
	SMAADemo(SMAADemo &&) = delete;
	SMAADemo &operator=(SMAADemo &&) = delete;

public:

	SMAADemo();

	~SMAADemo();

	void parseCommandLine(int argc, char *argv[]);

	void initRender();

	void createFramebuffers();

	void createCubes();

	void colorCubes();

	void mainLoopIteration();

	bool shouldKeepGoing() const {
		return keepGoing;
	}

	void render();

	void drawGUI(uint64_t elapsed);
};


SMAADemo::SMAADemo()
: windowWidth(1280)
, windowHeight(720)
, vsync(true)
, fullscreen(false)
, recreateSwapchain(false)
, glDebug(false)
, cubePipeline(0)
, imagePipeline(0)
, guiPipeline(0)
, cubeVBO(0)
, cubeIBO(0)
, linearSampler(0)
, nearestSampler(0)
, cubePower(3)
, antialiasing(true)
, aaMethod(AAMethod::SMAA)
, areaTex(0)
, searchTex(0)
, imguiFontsTex(0)
, rotateCamera(false)
, cameraRotation(0.0f)
, lastTime(0)
, freq(0)
, rotationTime(0)
, debugMode(0)
, colorMode(0)
, rightShift(false)
, leftShift(false)
, random(1)
, fxaaQuality(maxFXAAQuality - 1)
, smaaQuality(maxSMAAQuality - 1)
, keepGoing(true)
, activeScene(0)
{
	std::fill(rendertargets.begin(), rendertargets.end(), 0);

	freq = SDL_GetPerformanceFrequency();
	lastTime = SDL_GetPerformanceCounter();

	// TODO: detect screens, log interesting display parameters etc
	// TODO: initialize random using external source
}


SMAADemo::~SMAADemo() {
	ImGui::Shutdown();

	renderer.deleteBuffer(cubeVBO);
	renderer.deleteBuffer(cubeIBO);

	renderer.deleteSampler(linearSampler);
	renderer.deleteSampler(nearestSampler);

	renderer.deleteTexture(areaTex);
	renderer.deleteTexture(searchTex);
}


struct Vertex {
	float x, y, z;
};


const float coord = sqrtf(3.0f) / 2.0f;


static const Vertex vertices[] =
{
	  { -coord , -coord, -coord }
	, { -coord ,  coord, -coord }
	, {  coord , -coord, -coord }
	, {  coord ,  coord, -coord }
	, { -coord , -coord,  coord }
	, { -coord ,  coord,  coord }
	, {  coord , -coord,  coord }
	, {  coord ,  coord,  coord }
};


static const uint32_t indices[] =
{
	// top
	  1, 3, 5
	, 5, 3, 7

	// front
	, 0, 2, 1
	, 1, 2, 3

	// back
	, 7, 6, 5
	, 5, 6, 4

	// left
	, 0, 1, 4
	, 4, 1, 5

	// right
	, 2, 6, 3
	, 3, 6, 7

	// bottom
	, 2, 0, 6
	, 6, 0, 4
};


#define VBO_OFFSETOF(st, member) reinterpret_cast<GLvoid *>(offsetof(st, member))


void SMAADemo::parseCommandLine(int argc, char *argv[]) {
	try {
		TCLAP::CmdLine cmd("SMAA demo", ' ', "1.0");

		TCLAP::SwitchArg glDebugSwitch("", "gldebug", "Enable OpenGL debugging", cmd, false);
		TCLAP::ValueArg<unsigned int> windowWidthSwitch("", "width", "Window width", false, windowWidth, "width", cmd);
		TCLAP::ValueArg<unsigned int> windowHeightSwitch("", "height", "Window height", false, windowHeight, "height", cmd);
		TCLAP::UnlabeledMultiArg<std::string> imagesArg("images", "image files", false, "image file", cmd, true, nullptr);

		cmd.parse(argc, argv);

		glDebug = glDebugSwitch.getValue();
		windowWidth = windowWidthSwitch.getValue();
		windowHeight = windowHeightSwitch.getValue();

		const auto &imageFiles = imagesArg.getValue();
		images.reserve(imageFiles.size());
		for (const auto &filename : imageFiles) {
			images.push_back(Image());
			auto &img = images.back();
			img.tex = 0;
			img.filename = filename;
		}

	} catch (TCLAP::ArgException &e) {
		printf("parseCommandLine exception: %s for arg %s\n", e.error().c_str(), e.argId().c_str());
	} catch (...) {
		printf("parseCommandLine: unknown exception\n");
	}
}


void SMAADemo::initRender() {
	RendererDesc desc;
	desc.debug                = glDebug;
	desc.swapchain.fullscreen = fullscreen;
	desc.swapchain.width      = windowWidth;
	desc.swapchain.height     = windowHeight;
	desc.swapchain.vsync      = vsync;

	renderer = Renderer::createRenderer(desc);

	PipelineDesc plDesc;
	plDesc.depthWrite(false)
	      .depthTest(false)
	      .cullFaces(true);

	// all shader stages are affected by quality (SMAA_MAX_SEARCH_STEPS)
	// TODO: fix that
	// final blend is not affected by quality, TODO: check
	// if so, share
	for (unsigned int i = 0; i < maxSMAAQuality; i++) {
		ShaderMacros macros;
		std::string qualityString(std::string("SMAA_PRESET_") + smaaQualityLevels[i]);
		macros.emplace(qualityString, "1");

        auto vertexShader   = renderer.createVertexShader("smaaEdge", macros);
        auto fragmentShader = renderer.createFragmentShader("smaaEdge", macros);

		plDesc.vertexShader(vertexShader)
		      .fragmentShader(fragmentShader);
		smaaEdgePipelines[i]       = renderer.createPipeline(plDesc);

		vertexShader                = renderer.createVertexShader("smaaBlendWeight", macros);
		fragmentShader              = renderer.createFragmentShader("smaaBlendWeight", macros);
		plDesc.vertexShader(vertexShader)
		      .fragmentShader(fragmentShader);
		smaaBlendWeightPipelines[i] = renderer.createPipeline(plDesc);

		vertexShader                = renderer.createVertexShader("smaaNeighbor", macros);
		fragmentShader              = renderer.createFragmentShader("smaaNeighbor", macros);
		plDesc.vertexShader(vertexShader)
		      .fragmentShader(fragmentShader);
		smaaNeighborPipelines[i]   = renderer.createPipeline(plDesc);
	}

	ShaderMacros macros;

	// TODO: vertex shader not affected by quality, share it
	for (unsigned int i = 0; i < maxFXAAQuality; i++) {
		std::string qualityString(fxaaQualityLevels[i]);

		macros.emplace("FXAA_QUALITY_PRESET", qualityString);
		auto vertexShader   = renderer.createVertexShader("fxaa", macros);
		auto fragmentShader = renderer.createFragmentShader("fxaa", macros);
		plDesc.vertexShader(vertexShader)
		      .fragmentShader(fragmentShader);
		fxaaPipelines[i] = renderer.createPipeline(plDesc);
	}

	macros.clear();

	auto vertexShader   = renderer.createVertexShader("cube", macros);
	auto fragmentShader = renderer.createFragmentShader("cube", macros);

	cubePipeline = renderer.createPipeline(PipelineDesc()
	                                        .vertexShader(vertexShader)
	                                        .fragmentShader(fragmentShader)
	                                        .vertexAttrib(ATTR_POS, 0, 3, VtxFormat::Float, 0)
	                                        .vertexBufferStride(ATTR_POS, sizeof(Vertex))
	                                        .depthWrite(true)
	                                        .depthTest(true)
	                                        .cullFaces(true)
	                                       );

	vertexShader   = renderer.createVertexShader("image", macros);
	fragmentShader = renderer.createFragmentShader("image", macros);

	plDesc.vertexShader(vertexShader)
	      .fragmentShader(fragmentShader);
	plDesc.depthWrite(false)
	      .depthTest(false)
	      .cullFaces(true);

	imagePipeline = renderer.createPipeline(plDesc);

	macros.clear();

	vertexShader   = renderer.createVertexShader("gui", macros);
	fragmentShader = renderer.createFragmentShader("gui", macros);
	plDesc.vertexShader(vertexShader)
	      .fragmentShader(fragmentShader)
	      .cullFaces(false)
	      .blending(true)
	      .scissorTest(true);
	plDesc.vertexAttrib(ATTR_POS,   0, 2, VtxFormat::Float,  offsetof(ImDrawVert, pos))
	      .vertexAttrib(ATTR_UV,    0, 2, VtxFormat::Float,  offsetof(ImDrawVert, uv))
	      .vertexAttrib(ATTR_COLOR, 0, 4, VtxFormat::UNorm8, offsetof(ImDrawVert, col))
	      .vertexBufferStride(ATTR_POS, sizeof(ImDrawVert));
	guiPipeline = renderer.createPipeline(plDesc);

	linearSampler  = renderer.createSampler(SamplerDesc().minFilter(Linear). magFilter(Linear));
	nearestSampler = renderer.createSampler(SamplerDesc().minFilter(Nearest).magFilter(Nearest));

	cubeVBO = renderer.createBuffer(sizeof(vertices), &vertices[0]);
	cubeIBO = renderer.createBuffer(sizeof(indices), &indices[0]);

	const bool flipSMAATextures = true;

	TextureDesc texDesc;
	texDesc.width(AREATEX_WIDTH)
	       .height(AREATEX_HEIGHT)
	       .format(RG8);

	if (flipSMAATextures) {
		std::vector<unsigned char> tempBuffer(AREATEX_SIZE);
		for (unsigned int y = 0; y < AREATEX_HEIGHT; y++) {
			unsigned int srcY = AREATEX_HEIGHT - 1 - y;
			//unsigned int srcY = y;
			memcpy(&tempBuffer[y * AREATEX_PITCH], areaTexBytes + srcY * AREATEX_PITCH, AREATEX_PITCH);
		}
		texDesc.mipLevelData(0, &tempBuffer[0]);
		areaTex = renderer.createTexture(texDesc);
	} else {
		texDesc.mipLevelData(0, areaTexBytes);
		areaTex = renderer.createTexture(texDesc);
	}

	texDesc.width(SEARCHTEX_WIDTH)
	       .height(SEARCHTEX_HEIGHT)
	       .format(R8);
	if (flipSMAATextures) {
		std::vector<unsigned char> tempBuffer(SEARCHTEX_SIZE);
		for (unsigned int y = 0; y < SEARCHTEX_HEIGHT; y++) {
			unsigned int srcY = SEARCHTEX_HEIGHT - 1 - y;
			//unsigned int srcY = y;
			memcpy(&tempBuffer[y * SEARCHTEX_PITCH], searchTexBytes + srcY * SEARCHTEX_PITCH, SEARCHTEX_PITCH);
		}
		texDesc.mipLevelData(0, &tempBuffer[0]);
		searchTex = renderer.createTexture(texDesc);
	} else {
		texDesc.mipLevelData(0, searchTexBytes);
		searchTex = renderer.createTexture(texDesc);
	}

	createFramebuffers();

	for (auto &img : images) {
		const auto filename = img.filename.c_str();
		int width = 0, height = 0;
		unsigned char *imageData = stbi_load(filename, &width, &height, NULL, 3);
		printf(" %p  %dx%d\n", imageData, width, height);

		texDesc.width(width)
		       .height(height)
		       .format(RGB8);

		texDesc.mipLevelData(0, imageData);
		img.tex = renderer.createTexture(texDesc);

		stbi_image_free(imageData);
	}

	// default scene to last image or cubes if none
	activeScene = images.size();

	// imgui setup
	{
		ImGuiIO& io = ImGui::GetIO();
		io.KeyMap[ImGuiKey_Tab]        = SDLK_TAB;                     // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
		io.KeyMap[ImGuiKey_LeftArrow]  = SDL_SCANCODE_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow]    = SDL_SCANCODE_UP;
		io.KeyMap[ImGuiKey_DownArrow]  = SDL_SCANCODE_DOWN;
		io.KeyMap[ImGuiKey_PageUp]     = SDL_SCANCODE_PAGEUP;
		io.KeyMap[ImGuiKey_PageDown]   = SDL_SCANCODE_PAGEDOWN;
		io.KeyMap[ImGuiKey_Home]       = SDL_SCANCODE_HOME;
		io.KeyMap[ImGuiKey_End]        = SDL_SCANCODE_END;
		io.KeyMap[ImGuiKey_Delete]     = SDLK_DELETE;
		io.KeyMap[ImGuiKey_Backspace]  = SDLK_BACKSPACE;
		io.KeyMap[ImGuiKey_Enter]      = SDLK_RETURN;
		io.KeyMap[ImGuiKey_Escape]     = SDLK_ESCAPE;
		io.KeyMap[ImGuiKey_A]          = SDLK_a;
		io.KeyMap[ImGuiKey_C]          = SDLK_c;
		io.KeyMap[ImGuiKey_V]          = SDLK_v;
		io.KeyMap[ImGuiKey_X]          = SDLK_x;
		io.KeyMap[ImGuiKey_Y]          = SDLK_y;
		io.KeyMap[ImGuiKey_Z]          = SDLK_z;

		// TODO: clipboard
		io.SetClipboardTextFn = nullptr;
		io.GetClipboardTextFn = nullptr;
		io.ClipboardUserData  = nullptr;

		// Build texture atlas
		unsigned char *pixels = nullptr;
		int width = 0, height = 0;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		texDesc.width(width)
		       .height(height)
		       .format(RGBA8)
		       .mipLevelData(0, pixels);
		imguiFontsTex = renderer.createTexture(texDesc);
		io.Fonts->TexID = nullptr;
	}
}


void SMAADemo::createFramebuffers()	{
	if (fbos[0]) {
		for (unsigned int i = 0; i < Framebuffers::Count; i++) {
			assert(fbos[i]);
			renderer.deleteFramebuffer(fbos[i]);
		}

		for (unsigned int i = 0; i < RenderTargets::Count; i++) {
			assert(rendertargets[i]);
			renderer.deleteRenderTarget(rendertargets[i]);
		}
	}

	RenderTargetDesc rtDesc;

	rtDesc.width(windowWidth).height(windowHeight).format(RGBA8);
	rendertargets[RenderTargets::MainColor] = renderer.createRenderTarget(rtDesc);

	rtDesc.width(windowWidth).height(windowHeight).format(RGBA8);
	rendertargets[RenderTargets::FinalRender] = renderer.createRenderTarget(rtDesc);

	rtDesc.format(Depth16);
	rendertargets[RenderTargets::MainDepth] = renderer.createRenderTarget(rtDesc);

	FramebufferDesc fbDesc;
	fbDesc.depthStencil(rendertargets[RenderTargets::MainDepth]).color(0, rendertargets[RenderTargets::MainColor]);
	fbos[Framebuffers::MainRender] = renderer.createFramebuffer(fbDesc);

	fbDesc.depthStencil(0).color(0, rendertargets[RenderTargets::FinalRender]);
	fbos[Framebuffers::FinalRender] = renderer.createFramebuffer(fbDesc);

	// SMAA edges texture and FBO
	rtDesc.width(windowWidth).height(windowHeight).format(RGBA8);
	rendertargets[RenderTargets::Edges] = renderer.createRenderTarget(rtDesc);
	fbDesc.depthStencil(0).color(0, rendertargets[RenderTargets::Edges]);
	fbos[Framebuffers::Edges] = renderer.createFramebuffer(fbDesc);

	// SMAA blending weights texture and FBO
	rendertargets[RenderTargets::BlendWeights] = renderer.createRenderTarget(rtDesc);
	fbDesc.depthStencil(0).color(0, rendertargets[RenderTargets::BlendWeights]);
	fbos[Framebuffers::BlendWeights] = renderer.createFramebuffer(fbDesc);
}


void SMAADemo::createCubes() {
	// cubes on a side is some power of 2
	const unsigned int cubesSide = pow(2, cubePower);

	// cube of cubes, n^3 cubes total
	const unsigned int numCubes = pow(cubesSide, 3);

	const float cubeDiameter = sqrtf(3.0f);
	const float cubeDistance = cubeDiameter + 1.0f;

	const float bigCubeSide = cubeDistance * cubesSide;

	cubes.clear();
	cubes.reserve(numCubes);

	for (unsigned int x = 0; x < cubesSide; x++) {
		for (unsigned int y = 0; y < cubesSide; y++) {
			for (unsigned int z = 0; z < cubesSide; z++) {
				float qx = random.randFloat();
				float qy = random.randFloat();
				float qz = random.randFloat();
				float qw = random.randFloat();
				float reciprocLen = 1.0f / sqrtf(qx*qx + qy*qy + qz*qz + qw*qw);
				qx *= reciprocLen;
				qy *= reciprocLen;
				qz *= reciprocLen;
				qw *= reciprocLen;

				ShaderDefines::Cube cube;
				cube.position = glm::vec3((x * cubeDistance) - (bigCubeSide / 2.0f)
				                        , (y * cubeDistance) - (bigCubeSide / 2.0f)
				                        , (z * cubeDistance) - (bigCubeSide / 2.0f));

				cube.rotation = glm::vec4(qx, qy, qz, qw);
				cube.color = white.val;
				cubes.emplace_back(cube);
			}
		}
	}

	colorCubes();
}


void SMAADemo::colorCubes() {
	if (colorMode == 0) {
		for (auto &cube : cubes) {
			Color col;
			// random RGB, alpha = 1.0
			// FIXME: we're abusing little-endianness, make it portable
			col.val = random.randU32() | 0xFF000000;
			cube.color = col.val;
		}
	} else {
		for (auto &cube : cubes) {
			Color col;
			// YCbCr, fixed luma, random chroma, alpha = 1.0
			// worst case scenario for luma edge detection
			// TODO: use the same luma as shader

			float y = 0.5f;
			const float c_red = 0.299
				, c_green = 0.587
			, c_blue = 0.114;
			float cb = random.randFloat();
			float cr = random.randFloat();

			float r = cr * (2 - 2 * c_red) + y;
			float g = (y - c_blue * cb - c_red * cr) / c_green;
			float b = cb * (2 - 2 * c_blue) + y;

			col.r = 255 * r;
			col.g = 255 * g;
			col.b = 255 * b;
			col.a = 0xFF;
			cube.color = col.val;
		}
	}
}


static void printHelp() {
	printf(" a     - toggle antialiasing on/off\n");
	printf(" c     - re-color cubes\n");
	printf(" d     - cycle through debug visualizations\n");
	printf(" f     - toggle fullscreen\n");
	printf(" h     - print help\n");
	printf(" m     - change antialiasing method\n");
	printf(" q     - cycle through AA quality levels\n");
	printf(" v     - toggle vsync\n");
	printf(" SPACE - toggle camera rotation\n");
	printf(" ESC   - quit\n");
}


void SMAADemo::mainLoopIteration() {
	ImGuiIO& io = ImGui::GetIO();

	// TODO: timing
	SDL_Event event;
	memset(&event, 0, sizeof(SDL_Event));
	while (SDL_PollEvent(&event)) {
		int sceneIncrement = 1;
		switch (event.type) {
		case SDL_QUIT:
			keepGoing = false;
			break;

		case SDL_KEYDOWN:
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_ESCAPE:
				keepGoing = false;
				break;

			case SDL_SCANCODE_LSHIFT:
				leftShift = true;
				break;

			case SDL_SCANCODE_RSHIFT:
				rightShift = true;
				break;

			case SDL_SCANCODE_SPACE:
				rotateCamera = !rotateCamera;
				printf("camera rotation is %s\n", rotateCamera ? "on" : "off");
				break;

			case SDL_SCANCODE_A:
				antialiasing = !antialiasing;
				printf("antialiasing set to %s\n", antialiasing ? "on" : "off");
				break;

			case SDL_SCANCODE_C:
				if (rightShift || leftShift) {
					colorMode = (colorMode + 1) % 2;
					printf("color mode set to %s\n", colorMode ? "YCbCr" : "RGB");
				}
				colorCubes();
				break;

			case SDL_SCANCODE_D:
				if (antialiasing && aaMethod == AAMethod::SMAA) {
					if (leftShift || rightShift) {
						debugMode = (debugMode + 3 - 1) % 3;
					} else {
						debugMode = (debugMode + 1) % 3;
					}
					printf("Debug mode set to %s\n", smaaDebugModeStr(debugMode));
				}
				break;

			case SDL_SCANCODE_H:
				printHelp();
				break;

			case SDL_SCANCODE_M:
				aaMethod = AAMethod::AAMethod((int(aaMethod) + 1) % (int(AAMethod::LAST) + 1));
				printf("aa method set to %s\n", AAMethod::name(aaMethod));
				break;

			case SDL_SCANCODE_Q:
				switch (aaMethod) {
				case AAMethod::FXAA:
					if (leftShift || rightShift) {
						fxaaQuality = fxaaQuality + maxFXAAQuality - 1;
					} else {
						fxaaQuality = fxaaQuality + 1;
					}
					fxaaQuality = fxaaQuality % maxFXAAQuality;
					printf("FXAA quality set to %s (%u)\n", fxaaQualityLevels[fxaaQuality], fxaaQuality);
					break;

				case AAMethod::SMAA:
					if (leftShift || rightShift) {
						smaaQuality = smaaQuality + maxSMAAQuality - 1;
					} else {
						smaaQuality = smaaQuality + 1;
					}
					smaaQuality = smaaQuality % maxSMAAQuality;
					printf("SMAA quality set to %s (%u)\n", smaaQualityLevels[smaaQuality], smaaQuality);
					break;

				}
				break;

			case SDL_SCANCODE_V:
				vsync = !vsync;
				recreateSwapchain = true;
				break;

			case SDL_SCANCODE_F:
				fullscreen = !fullscreen;
				recreateSwapchain = true;
				break;

			case SDL_SCANCODE_LEFT:
				sceneIncrement = -1;
				// fallthrough
			case SDL_SCANCODE_RIGHT:
				{
					// all images + cubes scene
					unsigned int numScenes = images.size() + 1;
					activeScene = (activeScene + sceneIncrement + numScenes) % numScenes;
				}
				break;

			default:
				break;
			}
			break;

		case SDL_KEYUP:
			switch (event.key.keysym.scancode) {
			case SDL_SCANCODE_LSHIFT:
				leftShift = false;
				break;

			case SDL_SCANCODE_RSHIFT:
				rightShift = false;
				break;

			default:
				break;
			}
			break;

		case SDL_WINDOWEVENT:
			switch (event.window.event) {
			case SDL_WINDOWEVENT_SIZE_CHANGED:
			case SDL_WINDOWEVENT_RESIZED:
				windowWidth  = event.window.data1;
				windowHeight = event.window.data2;
				recreateSwapchain = true;
				break;
			default:
				break;
			}

		case SDL_MOUSEMOTION:
			io.MousePos = ImVec2(event.motion.x, event.motion.y);
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (event.button.button < 6) {
				// SDL and imgui have left and middle in different order
				static const uint8_t SDLMouseLookup[5] = { 0, 2, 1, 3, 4 };
				io.MouseDown[SDLMouseLookup[event.button.button - 1]] = (event.button.state == SDL_PRESSED);
			}
			break;

		}
	}

	render();
}


void SMAADemo::render() {
	if (recreateSwapchain) {
		SwapchainDesc desc;
		desc.fullscreen = fullscreen;
		desc.width      = windowWidth;
		desc.height     = windowHeight;
		desc.vsync      = vsync;

		renderer.recreateSwapchain(desc);
		recreateSwapchain = false;

		createFramebuffers();
	}

	uint64_t ticks = SDL_GetPerformanceCounter();
	uint64_t elapsed = ticks - lastTime;

	lastTime = ticks;

	ShaderDefines::Globals globals;
	globals.screenSize = glm::vec4(1.0f / float(windowWidth), 1.0f / float(windowHeight), windowWidth, windowHeight);
	globals.guiOrtho   = glm::ortho(0.0f, float(windowWidth), float(windowHeight), 0.0f);

	renderer.beginFrame();

	renderer.setViewport(0, 0, windowWidth, windowHeight);

	renderer.beginRenderPass(fbos[Framebuffers::MainRender]);

	if (activeScene == 0) {
		if (rotateCamera) {
			rotationTime += elapsed;

			const uint64_t rotationPeriod = 30 * freq;
			rotationTime = rotationTime % rotationPeriod;
			cameraRotation = float(M_PI * 2.0f * rotationTime) / rotationPeriod;
		}
		glm::mat4 view = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -25.0f)), cameraRotation, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 proj = glm::perspective(float(65.0f * M_PI * 2.0f / 360.0f), float(windowWidth) / windowHeight, 0.1f, 100.0f);
		globals.viewProj = proj * view;
		BufferHandle globalsUBO = renderer.createEphemeralBuffer(sizeof(ShaderDefines::Globals), &globals);
		renderer.bindUniformBuffer(0, globalsUBO);

		renderer.bindPipeline(cubePipeline);

		renderer.bindVertexBuffer(0, cubeVBO, sizeof(Vertex));
		renderer.bindIndexBuffer(cubeIBO, false);

		BufferHandle instanceSSBO = renderer.createEphemeralBuffer(sizeof(ShaderDefines::Cube) * cubes.size(), &cubes[0]);
		renderer.bindStorageBuffer(0, instanceSSBO);

		renderer.drawIndexedInstanced(3 * 2 * 6, cubes.size());
	} else {
		BufferHandle globalsUBO = renderer.createEphemeralBuffer(sizeof(ShaderDefines::Globals), &globals);
		renderer.bindUniformBuffer(0, globalsUBO);

		renderer.bindPipeline(imagePipeline);

		assert(activeScene - 1 < images.size());
		const auto &image = images[activeScene - 1];
		renderer.bindTexture(TEXUNIT_COLOR, image.tex, nearestSampler);
		renderer.draw(0, 3);
		renderer.bindTexture(TEXUNIT_COLOR, rendertargets[RenderTargets::MainColor], linearSampler);
	}
	renderer.endRenderPass();

	if (antialiasing) {
		renderer.bindTexture(TEXUNIT_COLOR, rendertargets[RenderTargets::MainColor], linearSampler);

		switch (aaMethod) {
		case AAMethod::FXAA:
			renderer.beginRenderPass(fbos[Framebuffers::FinalRender]);
			renderer.bindPipeline(fxaaPipelines[fxaaQuality]);
			renderer.draw(0, 3);
			drawGUI(elapsed);
			renderer.endRenderPass();
			break;

		case AAMethod::SMAA:
			renderer.bindPipeline(smaaEdgePipelines[smaaQuality]);

			renderer.bindTexture(TEXUNIT_AREATEX, areaTex, linearSampler);
			renderer.bindTexture(TEXUNIT_SEARCHTEX, searchTex, linearSampler);

			if (debugMode == 1) {
				// detect edges only
				renderer.beginRenderPass(fbos[Framebuffers::FinalRender]);
				renderer.draw(0, 3);
				drawGUI(elapsed);
				renderer.endRenderPass();
				break;
			} else {
				renderer.beginRenderPass(fbos[Framebuffers::Edges]);
				renderer.draw(0, 3);
				renderer.endRenderPass();
			}

			renderer.bindTexture(TEXUNIT_EDGES, rendertargets[RenderTargets::Edges], linearSampler);

			renderer.bindPipeline(smaaBlendWeightPipelines[smaaQuality]);
			if (debugMode == 2) {
				// show blending weights
				renderer.beginRenderPass(fbos[Framebuffers::FinalRender]);
				renderer.draw(0, 3);
				drawGUI(elapsed);
				renderer.endRenderPass();
				break;
			} else {
				renderer.beginRenderPass(fbos[Framebuffers::BlendWeights]);
				renderer.draw(0, 3);
				renderer.endRenderPass();
			}

			// full effect
			renderer.bindTexture(TEXUNIT_BLEND, rendertargets[RenderTargets::BlendWeights], linearSampler);

			renderer.bindPipeline(smaaNeighborPipelines[smaaQuality]);
			renderer.beginRenderPass(fbos[Framebuffers::FinalRender]);
			renderer.draw(0, 3);
			drawGUI(elapsed);

			renderer.endRenderPass();
			break;
		}

	} else {
		renderer.beginRenderPass(fbos[Framebuffers::FinalRender]);
		// TODO: not necessary?
		renderer.blitFBO(fbos[Framebuffers::MainRender], fbos[Framebuffers::FinalRender]);
		drawGUI(elapsed);
		renderer.endRenderPass();
	}

	renderer.presentFrame(fbos[Framebuffers::FinalRender]);

}


void SMAADemo::drawGUI(uint64_t elapsed) {
	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = float(double(elapsed) / double(1000000000ULL));

	io.DisplaySize = ImVec2(windowWidth, windowHeight);
	io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

	ImGui::NewFrame();

	bool windowVisible = true;
	int flags = 0;
	flags |= ImGuiWindowFlags_NoTitleBar;

	if (ImGui::Begin("SMAA", &windowVisible, flags)) {
		ImGui::Checkbox("Antialiasing", &antialiasing);
		int aa = static_cast<int>(aaMethod);
		ImGui::RadioButton("FXAA", &aa, static_cast<int>(AAMethod::FXAA)); ImGui::SameLine();
		ImGui::RadioButton("SMAA", &aa, static_cast<int>(AAMethod::SMAA));
		aaMethod = static_cast<AAMethod::AAMethod>(aa);
	}

	ImGui::End();

#if 0
	bool demoVisible = true;
	ImGui::ShowTestWindow(&demoVisible);
#endif  // 0

	ImGui::Render();

	auto drawData = ImGui::GetDrawData();
	assert(drawData->Valid);
	if (drawData->CmdListsCount > 0) {
		assert(drawData->CmdLists      != nullptr);
		assert(drawData->TotalVtxCount >  0);
		assert(drawData->TotalIdxCount >  0);

		renderer.bindPipeline(guiPipeline);
		renderer.bindTexture(TEXUNIT_COLOR, imguiFontsTex, linearSampler);
		// TODO: upload all buffers first, render after
		// and one buffer each vertex/index

		for (int n = 0; n < drawData->CmdListsCount; n++) {
			const ImDrawList* cmd_list = drawData->CmdLists[n];

			BufferHandle vtxBuf = renderer.createEphemeralBuffer(cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data);
			BufferHandle idxBuf = renderer.createEphemeralBuffer(cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), cmd_list->IdxBuffer.Data);
			renderer.bindIndexBuffer(idxBuf, true);
			renderer.bindVertexBuffer(0, vtxBuf, sizeof(ImDrawVert));

			const ImDrawIdx* idx_buffer_offset = 0;
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback) {
					// TODO: this probably does nothing useful for us
					assert(false);

					pcmd->UserCallback(cmd_list, pcmd);
				} else {
					assert(pcmd->TextureId == 0);
					renderer.setScissorRect(pcmd->ClipRect.x, pcmd->ClipRect.w, pcmd->ClipRect.z - pcmd->ClipRect.x, pcmd->ClipRect.w - pcmd->ClipRect.y);
					// TODO: drawIndexed without instance
					// and with offset
					renderer.drawIndexedInstanced(pcmd->ElemCount * 3, 1);
				}
				idx_buffer_offset += pcmd->ElemCount;
			}
		}
#if 0
		printf("CmdListsCount: %d\n", drawData->CmdListsCount);
		printf("TotalVtxCount: %d\n", drawData->TotalVtxCount);
		printf("TotalIdxCount: %d\n", drawData->TotalIdxCount);
#endif // 0
	} else {
		assert(drawData->CmdLists      == nullptr);
		assert(drawData->TotalVtxCount == 0);
		assert(drawData->TotalIdxCount == 0);
	}
}


int main(int argc, char *argv[]) {
	try {
		auto demo = std::make_unique<SMAADemo>();

		demo->parseCommandLine(argc, argv);

		demo->initRender();
		demo->createCubes();
		printHelp();

		while (demo->shouldKeepGoing()) {
			demo->mainLoopIteration();
		}
	} catch (std::exception &e) {
		printf("caught std::exception \"%s\"\n", e.what());
		// so native dumps core
		throw;
	} catch (...) {
		printf("unknown exception\n");
		// so native dumps core
		throw;
	}
	return 0;
}
