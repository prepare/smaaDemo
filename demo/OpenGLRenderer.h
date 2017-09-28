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


#ifndef OPENGLRENDERER_H
#define OPENGLRENDERER_H


#include <GL/glew.h>


struct ShaderResource {
	unsigned int    set;
	unsigned int    binding;
	DescriptorType  type;
};


struct DescriptorSetLayout {
	std::vector<DescriptorLayout> layout;
};


struct Pipeline {
	PipelineDesc  desc;
	GLuint        shader;


	Pipeline(const Pipeline &)            = delete;
	Pipeline &operator=(const Pipeline &) = delete;

	Pipeline(Pipeline &&)            = default;
	Pipeline &operator=(Pipeline &&) = default;

	Pipeline();

	~Pipeline();
};


struct Buffer {
	GLuint        buffer;
	bool          ringBufferAlloc;
	unsigned int  beginOffs;
	unsigned int  size;
	// TODO: usage flags for debugging


	Buffer(const Buffer &)            = delete;
	Buffer &operator=(const Buffer &) = delete;

	Buffer(Buffer &&other)
	: buffer(other.buffer)
	, ringBufferAlloc(other.ringBufferAlloc)
	, beginOffs(other.beginOffs)
	, size(other.size)
	{
		other.buffer          = 0;
		other.ringBufferAlloc = false;
		other.beginOffs       = 0;
		other.size            = 0;
	}

	Buffer &operator=(Buffer &&other) {
		buffer                = other.buffer;
		other.buffer          = 0;

		ringBufferAlloc       = other.ringBufferAlloc;
		other.ringBufferAlloc = false;

		beginOffs             = other.beginOffs;
		other.beginOffs       = 0;

		size                  = other.size;
		other.size            = 0;

		return *this;
	}

	Buffer();

	~Buffer();
};


struct VertexShader {
	GLuint shader;
	std::string name;
	std::vector<ShaderResource> resources;


	VertexShader(const VertexShader &) = delete;
	VertexShader &operator=(const VertexShader &) = delete;

	VertexShader(VertexShader &&)            = default;
	VertexShader &operator=(VertexShader &&) = default;

	VertexShader();

	~VertexShader();
};


struct FragmentShader {
	GLuint shader;
	std::string name;
	std::vector<ShaderResource> resources;


	FragmentShader(const FragmentShader &) = delete;
	FragmentShader &operator=(const FragmentShader &) = delete;

	FragmentShader(FragmentShader &&)            = default;
	FragmentShader &operator=(FragmentShader &&) = default;


	FragmentShader();

	~FragmentShader();
};


struct RenderTarget {
	GLuint readFBO;
	unsigned int width, height;
	Layout               currentLayout;
	TextureHandle        texture;
	Format               format;


	RenderTarget()
	: readFBO(0)
	, width(0)
	, height(0)
	, currentLayout(Layout::Invalid)
	{
	}

	RenderTarget(const RenderTarget &)            = delete;
	RenderTarget &operator=(const RenderTarget &) = delete;

	RenderTarget(RenderTarget &&other)
	: readFBO(other.readFBO)
	, width(other.width)
	, height(other.height)
	, currentLayout(other.currentLayout)
	, texture(other.texture)   // TODO: use std::move
	, format(other.format)
	{
		other.readFBO = 0;
		other.width  = 0;
		other.height = 0;
		other.currentLayout = Layout::Invalid;
		other.texture       = TextureHandle();
		other.format        = Invalid;
	}

	RenderTarget &operator=(RenderTarget &&other) {
		if (this == &other) {
			return *this;
		}

		assert(!readFBO);
		assert(!texture);

		readFBO       = other.readFBO;
		width         = other.width;
		height        = other.height;
		currentLayout = other.currentLayout;
		texture       = other.texture;
		format        = other.format;

		other.readFBO       = 0;
		other.width         = 0;
		other.height        = 0;
		other.currentLayout = Layout::Invalid;
		other.texture       = TextureHandle();
		other.format        = Invalid;

		return *this;
	};

	~RenderTarget() {
		assert(readFBO  == 0);
		assert(!texture);
	}

};


struct Framebuffer {
	GLuint fbo;
	RenderTargetHandle                                       depthStencil;
	std::array<RenderTargetHandle, MAX_COLOR_RENDERTARGETS>  colors;
	RenderPassHandle                                         renderPass;

	unsigned int width, height;


	Framebuffer(const Framebuffer &)            = delete;
	Framebuffer &operator=(const Framebuffer &) = delete;

	Framebuffer(Framebuffer &&other)
	: fbo(other.fbo)
	, depthStencil(other.depthStencil)
	, width(other.width)
	, height(other.height)
	{
		other.fbo      = 0;
		other.width    = 0;
		other.height   = 0;
		// TODO: use std::move
		assert(!other.colors[1]);
		this->colors[0]    = other.colors[0];
		other.colors[0]    = RenderTargetHandle();
		other.depthStencil = RenderTargetHandle();
	}

	Framebuffer &operator=(Framebuffer &&other) = delete;

	Framebuffer()
	: fbo(0)
	, width(0)
	, height(0)
	{
	}

	~Framebuffer();
};


struct RenderPass {
	RenderPassDesc  desc;


	RenderPass(const RenderPass &) = delete;
	RenderPass &operator=(const RenderPass &) = delete;

	RenderPass(RenderPass &&other)
	: desc(other.desc)
	{
	}

	RenderPass &operator=(RenderPass &&other) = delete;

	RenderPass()
	{
	}


	~RenderPass();
};


struct Sampler {
	GLuint sampler;


	Sampler(const Sampler &) = delete;
	Sampler &operator=(const Sampler &) = delete;

	Sampler(Sampler &&other)
	: sampler(other.sampler)
	{
		other.sampler = 0;
	}

	Sampler &operator=(Sampler &&other) {
		std::swap(sampler, other.sampler);

		return *this;
	}

	Sampler()
	: sampler(0)
	{
	}


	~Sampler()
	{
		assert(sampler == 0);
	}
};


struct Texture {
	// TODO: need target for anything?
	GLuint tex;
	unsigned int width, height;
	bool         renderTarget;


	Texture(const Texture &) = delete;
	Texture &operator=(const Texture &) = delete;

	Texture(Texture &&other)
	: tex(other.tex)
	, width(other.width)
	, height(other.height)
	, renderTarget(other.renderTarget)
	{
		other.tex      = 0;
		other.width    = 0;
		other.height   = 0;
		other.renderTarget = false;
	}

	Texture &operator=(Texture &&other) {
		std::swap(tex,    other.tex);
		std::swap(width,  other.width);
		std::swap(height, other.height);
		std::swap(renderTarget, other.renderTarget);

		return *this;
	}

	Texture()
	: tex(0)
	, width(0)
	, height(0)
	, renderTarget(false)
	{
	}


	~Texture();
};


struct RendererBase {
	GLuint        ringBuffer;
	bool          persistentMapInUse;
	char         *persistentMapping;

	PipelineDesc  currentPipeline;
	RenderPassHandle  currentRenderPass;
	FramebufferHandle currentFramebuffer;

	SDL_Window *window;
	SDL_GLContext context;

	bool           debug;
	GLuint vao;
	bool idxBuf16Bit;
	unsigned int  indexBufByteOffset;

	unsigned int   uboAlign;
	unsigned int   ssboAlign;

	ResourceContainer<Buffer>               buffers;
	ResourceContainer<DescriptorSetLayout>  dsLayouts;
	ResourceContainer<FragmentShader>       fragmentShaders;
	ResourceContainer<Framebuffer>          framebuffers;
	ResourceContainer<Pipeline>             pipelines;
	ResourceContainer<RenderPass>           renderPasses;
	ResourceContainer<RenderTarget>         renderTargets;
	ResourceContainer<Sampler>              samplers;
	ResourceContainer<Texture>              textures;
	ResourceContainer<VertexShader>         vertexShaders;

	std::vector<BufferHandle> ephemeralBuffers;


	RendererBase();

	~RendererBase();
};


#endif  // OPENGLRENDERER_H
