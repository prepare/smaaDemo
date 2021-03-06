/*
Copyright (c) 2015-2017 Alternative Games Ltd / Turo Lamminen

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


#ifndef RENDERER_H
#define RENDERER_H


#include <string>
#include <unordered_map>
#include <array>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#include <glm/glm.hpp>

#include <SDL.h>


namespace renderer {


#define MAX_COLOR_RENDERTARGETS 2
#define MAX_VERTEX_ATTRIBS      4
#define MAX_VERTEX_BUFFERS      1
#define MAX_DESCRIPTOR_SETS     2  // per pipeline
#define MAX_TEXTURE_MIPLEVELS   14
#define MAX_TEXTURE_SIZE        (1 << (MAX_TEXTURE_MIPLEVELS - 1))


struct Buffer;
struct RendererImpl;
struct DescriptorSetLayout;
struct FragmentShader;
struct Framebuffer;
struct Pipeline;
struct RenderPass;
struct RenderTarget;
struct Sampler;
struct Texture;
struct VertexShader;


template <class T> class ResourceContainer;


template <class T>
class Handle {
	friend class ResourceContainer<T>;

	uint32_t handle;


	explicit Handle(uint32_t handle_)
	: handle(handle_)
	{
	}

public:


	Handle()
	: handle(0)
	{
	}


	~Handle() {
	};


	Handle(const Handle<T> &other)
	: handle(other.handle)
	{
	}


	Handle &operator=(const Handle<T> &other) {
		handle = other.handle;

		return *this;
	}


	Handle(Handle<T> &&other)
	: handle(other.handle)
	{
		other.handle = 0;
	}


	Handle<T> &operator=(Handle<T> &&other) {
		handle       = other.handle;
		other.handle = 0;

		return *this;
	}


	bool operator==(const Handle<T> &other) const {
		return handle == other.handle;
	}


	explicit operator bool() const {
		return handle != 0;
	}
};


typedef Handle<Buffer>               BufferHandle;
typedef Handle<DescriptorSetLayout>  DSLayoutHandle;
typedef Handle<FragmentShader>       FragmentShaderHandle;
typedef Handle<Framebuffer>          FramebufferHandle;
typedef Handle<Pipeline>             PipelineHandle;
typedef Handle<RenderPass>           RenderPassHandle;
typedef Handle<RenderTarget>         RenderTargetHandle;
typedef Handle<Sampler>              SamplerHandle;
typedef Handle<Texture>              TextureHandle;
typedef Handle<VertexShader>         VertexShaderHandle;


enum class DescriptorType : uint8_t {
	  End
	, UniformBuffer
	, StorageBuffer
	, Sampler
	, Texture
	, CombinedSampler
	, Count
};


enum class FilterMode : uint8_t{
	  Nearest
	, Linear
};


enum class Format : uint8_t {
	  Invalid
	, R8
	, RG8
	, RGB8
	, RGBA8
	, sRGBA8
	, Depth16
	, Depth16S8
	, Depth24S8
	, Depth24X8
	, Depth32Float
};


enum class Layout : uint8_t {
	  Invalid
	, ShaderRead
	, TransferSrc
};


enum class VSync : uint8_t {
	  Off
	, On
	, LateSwapTear
};


enum class VtxFormat : uint8_t {
	  Float
	, UNorm8
};


enum class WrapMode : uint8_t {
	  Clamp
	, Wrap
};


// CombinedSampler helper
struct CSampler {
	TextureHandle tex;
	SamplerHandle sampler;
};


struct DescriptorLayout {
	DescriptorType  type;
	unsigned int    offset;
	// TODO: stage flags
};


struct MemoryStats {
	uint32_t allocationCount;
	uint32_t subAllocationCount;
	uint64_t usedBytes;
	uint64_t unusedBytes;


	MemoryStats()
	: allocationCount(0)
	, subAllocationCount(0)
	, usedBytes(0)
	, unusedBytes(0)
	{
	}

	~MemoryStats() {}

	MemoryStats(const MemoryStats &stats)            = default;
	MemoryStats(MemoryStats &&stats)                 = default;

	MemoryStats &operator=(const MemoryStats &stats) = default;
	MemoryStats &operator=(MemoryStats &&stats)      = default;
};


typedef std::unordered_map<std::string, std::string> ShaderMacros;


const char *formatName(Format format);


struct FramebufferDesc {
	FramebufferDesc()
	{
	}

	~FramebufferDesc() { }

	FramebufferDesc(const FramebufferDesc &)            = default;
	FramebufferDesc(FramebufferDesc &&)                 = default;

	FramebufferDesc &operator=(const FramebufferDesc &) = default;
	FramebufferDesc &operator=(FramebufferDesc &&)      = default;


	FramebufferDesc &renderPass(RenderPassHandle rp) {
		renderPass_ = rp;
		return *this;
	}

	FramebufferDesc &depthStencil(RenderTargetHandle ds) {
		depthStencil_ = ds;
		return *this;
	}

	FramebufferDesc &color(unsigned int index, RenderTargetHandle c) {
		assert(index < MAX_COLOR_RENDERTARGETS);
		colors_[index] = c;
		return *this;
	}

	FramebufferDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}


private:

	RenderPassHandle                                         renderPass_;
	RenderTargetHandle                                       depthStencil_;
	std::array<RenderTargetHandle, MAX_COLOR_RENDERTARGETS>  colors_;
	std::string                                              name_;

	friend struct RendererImpl;
};


class PipelineDesc {
	VertexShaderHandle    vertexShader_;
	FragmentShaderHandle  fragmentShader_;
	RenderPassHandle      renderPass_;
	uint32_t              vertexAttribMask;
	bool                  depthWrite_;
	bool                  depthTest_;
	bool                  cullFaces_;
	bool                  scissorTest_;
	bool                  blending_;
	// TODO: blend equation and function
	// TODO: per-MRT blending

	struct VertexAttr {
		uint8_t bufBinding;
		uint8_t count;
		VtxFormat format;
		uint8_t offset;
	};

	struct VertexBuf {
		uint32_t stride;
	};

	std::array<VertexAttr,     MAX_VERTEX_ATTRIBS>   vertexAttribs;
	std::array<VertexBuf,      MAX_VERTEX_BUFFERS>   vertexBuffers;
	std::array<DSLayoutHandle, MAX_DESCRIPTOR_SETS>  descriptorSetLayouts;

	std::string                                      name_;


public:

	PipelineDesc &vertexShader(VertexShaderHandle h) {
		vertexShader_ = h;
		return *this;
	}

	PipelineDesc &fragmentShader(FragmentShaderHandle h) {
		fragmentShader_ = h;
		return *this;
	}

	PipelineDesc &renderPass(RenderPassHandle h) {
		renderPass_ = h;
		return *this;
	}

	PipelineDesc &vertexAttrib(uint32_t attrib, uint8_t bufBinding, uint8_t count, VtxFormat format, uint8_t offset) {
		assert(attrib < MAX_VERTEX_ATTRIBS);

		vertexAttribs[attrib].bufBinding = bufBinding;
		vertexAttribs[attrib].count      = count;
		vertexAttribs[attrib].format     = format;
		vertexAttribs[attrib].offset     = offset;


		vertexAttribMask |= (1 << attrib);
		return *this;
	}

	PipelineDesc &vertexBufferStride(uint8_t buf, uint32_t stride) {
		assert(buf < MAX_VERTEX_BUFFERS);
		vertexBuffers[buf].stride = stride;

		return *this;
	}

	PipelineDesc &descriptorSetLayout(unsigned int index, DSLayoutHandle handle) {
		assert(index < MAX_DESCRIPTOR_SETS);
		descriptorSetLayouts[index] = handle;
		return *this;
	}

	template <typename T> PipelineDesc &descriptorSetLayout(unsigned int index) {
		assert(index < MAX_DESCRIPTOR_SETS);
		descriptorSetLayouts[index] = T::layoutHandle;
		return *this;
	}

	PipelineDesc &blending(bool b) {
		blending_ = b;
		return *this;
	}

	PipelineDesc &depthWrite(bool d) {
		depthWrite_ = d;
		return *this;
	}

	PipelineDesc &depthTest(bool d) {
		depthTest_ = d;
		return *this;
	}

	PipelineDesc &cullFaces(bool c) {
		cullFaces_ = c;
		return *this;
	}

	PipelineDesc &scissorTest(bool s) {
		scissorTest_ = s;
		return *this;
	}

	PipelineDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}

	PipelineDesc()
	: vertexAttribMask(0)
	, depthWrite_(false)
	, depthTest_(false)
	, cullFaces_(false)
	, scissorTest_(false)
	, blending_(false)
	{
		for (unsigned int i = 0; i < MAX_VERTEX_ATTRIBS; i++) {
			vertexAttribs[i].bufBinding = 0;
			vertexAttribs[i].count      = 0;
			vertexAttribs[i].offset     = 0;
		}

		for (unsigned int i = 0; i < MAX_VERTEX_BUFFERS; i++) {
			vertexBuffers[i].stride = 0;
		}
	}

	~PipelineDesc() {}

	PipelineDesc(const PipelineDesc &desc)            = default;
	PipelineDesc(PipelineDesc &&desc)                 = default;

	PipelineDesc &operator=(const PipelineDesc &desc) = default;
	PipelineDesc &operator=(PipelineDesc &&desc)      = default;

	friend struct RendererImpl;
};


struct RenderPassDesc {
	RenderPassDesc()
	: depthStencilFormat_(Format::Invalid)
	, colorFinalLayout_(Layout::ShaderRead)
	{
		std::fill(colorFormats_.begin(), colorFormats_.end(), Format::Invalid);
	}

	~RenderPassDesc() { }

	RenderPassDesc(const RenderPassDesc &)            = default;
	RenderPassDesc(RenderPassDesc &&)                 = default;

	RenderPassDesc &operator=(const RenderPassDesc &) = default;
	RenderPassDesc &operator=(RenderPassDesc &&)      = default;

	RenderPassDesc &depthStencil(Format ds) {
		depthStencilFormat_ = ds;
		return *this;
	}

	RenderPassDesc &color(unsigned int index, Format c) {
		assert(index < MAX_COLOR_RENDERTARGETS);
		colorFormats_[index] = c;
		return *this;
	}

	RenderPassDesc &colorFinalLayout(Layout l) {
		colorFinalLayout_ = l;
		return *this;
	}

	RenderPassDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}

private:

	Format                                       depthStencilFormat_;
	std::array<Format, MAX_COLOR_RENDERTARGETS>  colorFormats_;
	Layout                                       colorFinalLayout_;
	std::string                                  name_;

	friend struct RendererImpl;
};


struct RenderTargetDesc {
	RenderTargetDesc()
	: width_(0)
	, height_(0)
	, format_(Format::Invalid)
	, additionalViewFormat_(Format::Invalid)
	{
	}

	~RenderTargetDesc() { }

	RenderTargetDesc(const RenderTargetDesc &)            = default;
	RenderTargetDesc(RenderTargetDesc &&)                 = default;

	RenderTargetDesc &operator=(const RenderTargetDesc &) = default;
	RenderTargetDesc &operator=(RenderTargetDesc &&)      = default;


	RenderTargetDesc &width(unsigned int w) {
		assert(w < MAX_TEXTURE_SIZE);
		width_ = w;
		return *this;
	}

	RenderTargetDesc &height(unsigned int h) {
		assert(h < MAX_TEXTURE_SIZE);
		height_ = h;
		return *this;
	}

	RenderTargetDesc &format(Format f) {
		format_ = f;
		return *this;
	}

	RenderTargetDesc &additionalViewFormat(Format f) {
		additionalViewFormat_ = f;
		return *this;
	}

	RenderTargetDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}

private:

	unsigned int   width_, height_;
	// TODO: unsigned int multisample;
	Format         format_;
	Format         additionalViewFormat_;
	std::string    name_;

	friend struct RendererImpl;
};


struct SamplerDesc {
	SamplerDesc()
	: min(FilterMode::Nearest)
	, mag(FilterMode::Nearest)
	, wrapMode(WrapMode::Clamp)
	{
	}

	SamplerDesc(const SamplerDesc &desc)            = default;
	SamplerDesc(SamplerDesc &&desc)                 = default;

	SamplerDesc &operator=(const SamplerDesc &desc) = default;
	SamplerDesc &operator=(SamplerDesc &&desc)      = default;

	~SamplerDesc() {}

	SamplerDesc &minFilter(FilterMode m) {
		min = m;
		return *this;
	}

	SamplerDesc &magFilter(FilterMode m) {
		mag = m;
		return *this;
	}

	SamplerDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}

private:

	FilterMode  min, mag;
	WrapMode    wrapMode;
	std::string name_;

	friend struct RendererImpl;
};


struct SwapchainDesc {
	unsigned int  width, height;
	unsigned int  numFrames;
	VSync         vsync;
	bool          fullscreen;


	SwapchainDesc()
	: width(0)
	, height(0)
	, numFrames(3)
	, vsync(VSync::On)
	, fullscreen(false)
	{
	}
};


struct TextureDesc {
	TextureDesc()
	: width_(0)
	, height_(0)
	, numMips_(1)
	, format_(Format::Invalid)
	{
		std::fill(mipData_.begin(), mipData_.end(), MipLevel());
	}

	~TextureDesc() { }

	TextureDesc(const TextureDesc &)            = default;
	TextureDesc(TextureDesc &&)                 = default;

	TextureDesc &operator=(const TextureDesc &) = default;
	TextureDesc &operator=(TextureDesc &&)      = default;


	TextureDesc &width(unsigned int w) {
		assert(w < MAX_TEXTURE_SIZE);
		width_ = w;
		return *this;
	}

	TextureDesc &height(unsigned int h) {
		assert(h < MAX_TEXTURE_SIZE);
		height_ = h;
		return *this;
	}

	TextureDesc &format(Format f) {
		format_ = f;
		return *this;
	}

	TextureDesc &mipLevelData(unsigned int level, const void *data, unsigned int size) {
		assert(level < numMips_);
		mipData_[level].data = data;
		mipData_[level].size = size;
		return *this;
	}

	TextureDesc &name(const std::string &str) {
		name_ = str;
		return *this;
	}


private:

	struct MipLevel {
		const void   *data;
		unsigned int  size;

		MipLevel()
		: data(nullptr)
		, size(0)
		{
		}
	};

	unsigned int                                 width_, height_;
	unsigned int                                 numMips_;
	Format                                       format_;
	std::array<MipLevel, MAX_TEXTURE_MIPLEVELS>  mipData_;
	std::string                                  name_;

	friend struct RendererImpl;
};


struct RendererDesc {
	bool           debug;
	bool           tracing;
	bool           skipShaderCache;
	unsigned int   ephemeralRingBufSize;
	SwapchainDesc  swapchain;


	RendererDesc()
	: debug(false)
	, tracing(false)
	, skipShaderCache(false)
	, ephemeralRingBufSize(1 * 1048576)
	{
	}
};


class Renderer {
	RendererImpl *impl;

	explicit Renderer(RendererImpl *impl_)
	: impl(impl_)
	{
		assert(impl != nullptr);
	}
	


public:

	static Renderer createRenderer(const RendererDesc &desc);

	Renderer(const Renderer &)            = delete;
	Renderer &operator=(const Renderer &) = delete;

	Renderer &operator=(Renderer &&other)
	{
		if (this == &other) {
			return *this;
		}

		assert(!impl);

		impl       = other.impl;
		other.impl = nullptr;

		return *this;
	}

	Renderer(Renderer &&other)
	: impl(other.impl)
	{
		other.impl = nullptr;
	}

	Renderer()
	: impl(nullptr)
	{
	}

	~Renderer();




	bool isRenderTargetFormatSupported(Format format) const;
	unsigned int getCurrentRefreshRate() const;
	unsigned int getMaxRefreshRate() const;

	// TODO: add buffer usage flags
	BufferHandle          createBuffer(uint32_t size, const void *contents);
	BufferHandle          createEphemeralBuffer(uint32_t size, const void *contents);
	FragmentShaderHandle  createFragmentShader(const std::string &name, const ShaderMacros &macros);
	FramebufferHandle     createFramebuffer(const FramebufferDesc &desc);
	PipelineHandle        createPipeline(const PipelineDesc &desc);
	RenderPassHandle      createRenderPass(const RenderPassDesc &desc);
	RenderTargetHandle    createRenderTarget(const RenderTargetDesc &desc);
	SamplerHandle         createSampler(const SamplerDesc &desc);
	TextureHandle         createTexture(const TextureDesc &desc);
	VertexShaderHandle    createVertexShader(const std::string &name, const ShaderMacros &macros);
	// TODO: non-ephemeral descriptor set

	DSLayoutHandle createDescriptorSetLayout(const DescriptorLayout *layout);
	template <typename T> void registerDescriptorSetLayout() {
		T::layoutHandle = createDescriptorSetLayout(T::layout);
	}

	// gets the textures of a rendertarget to be used for sampling
	// might be ephemeral, don't store
	TextureHandle        getRenderTargetTexture(RenderTargetHandle handle);
	TextureHandle        getRenderTargetView(RenderTargetHandle handle, Format f);

	void deleteBuffer(BufferHandle handle);
	void deleteFramebuffer(FramebufferHandle fbo);
	void deleteRenderPass(RenderPassHandle fbo);
	void deleteRenderTarget(RenderTargetHandle &fbo);
	void deleteSampler(SamplerHandle handle);
	void deleteTexture(TextureHandle handle);


	void setSwapchainDesc(const SwapchainDesc &desc);
	glm::uvec2 getDrawableSize() const;
	MemoryStats getMemStats() const;

	// rendering
	void beginFrame();
	void presentFrame(RenderTargetHandle image);

	void beginRenderPass(RenderPassHandle rpHandle, FramebufferHandle fbHandle);
	void endRenderPass();

	void setScissorRect(unsigned int x, unsigned int y, unsigned int width, unsigned int height);
	void setViewport(unsigned int x, unsigned int y, unsigned int width, unsigned int height);

	void bindPipeline(PipelineHandle pipeline);
	void bindDescriptorSet(unsigned int index, const DSLayoutHandle layout, const void *data);
	template <typename T> void bindDescriptorSet(unsigned int index, const T &data) {
		bindDescriptorSet(index, T::layoutHandle, &data);
	}

	void bindIndexBuffer(BufferHandle buffer, bool bit16);
	void bindVertexBuffer(unsigned int binding, BufferHandle buffer);

	void draw(unsigned int firstVertex, unsigned int vertexCount);
	void drawIndexedInstanced(unsigned int vertexCount, unsigned int instanceCount);
	void drawIndexedOffset(unsigned int vertexCount, unsigned int firstIndex);
};


}  // namespace renderer


#endif  // RENDERER_H
