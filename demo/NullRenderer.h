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


#ifndef NULLRENDERER_H
#define NULLRENDERER_H


namespace renderer {


struct Buffer {
	bool          ringBufferAlloc;
	unsigned int  beginOffs;
	unsigned int  size;
	// TODO: usage flags for debugging


	Buffer(const Buffer &)            = delete;
	Buffer &operator=(const Buffer &) = delete;

	Buffer(Buffer &&other)
	: ringBufferAlloc(other.ringBufferAlloc)
	, beginOffs(other.beginOffs)
	, size(other.size)
	{
		other.ringBufferAlloc = false;
		other.beginOffs       = 0;
		other.size            = 0;
	}

	Buffer &operator=(Buffer &&other) {
		if (this == &other) {
			return *this;
		}

		ringBufferAlloc       = other.ringBufferAlloc;
		beginOffs             = other.beginOffs;
		size                  = other.size;

		other.ringBufferAlloc = false;
		other.beginOffs       = 0;
		other.size            = 0;

		return *this;
	}

	Buffer();

	~Buffer();
};


struct DescriptorSetLayout {
	std::vector<DescriptorLayout> layout;


	DescriptorSetLayout() {}

	DescriptorSetLayout(const DescriptorSetLayout &)            = delete;
	DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

	DescriptorSetLayout(DescriptorSetLayout &&other)
	: layout(std::move(other.layout))
	{
		assert(other.layout.empty());
	}

	DescriptorSetLayout &operator=(DescriptorSetLayout &&other) {
		if (this == &other) {
			return *this;
		}

		assert(layout.empty());
		layout = std::move(other.layout);
		assert(other.layout.empty());

		return *this;
	}

	~DescriptorSetLayout() {}
};


struct Framebuffer {
	RenderPassHandle  renderPass;


	Framebuffer(const Framebuffer &)            = default;
	Framebuffer &operator=(const Framebuffer &) = default;

	Framebuffer(Framebuffer &&other)
	: renderPass(other.renderPass)
	{
		other.renderPass = RenderPassHandle();
	}

	Framebuffer &operator=(Framebuffer &&other) {
		if (this == &other) {
			return *this;
		}

		assert(!renderPass);

		renderPass       = other.renderPass;

		other.renderPass = RenderPassHandle();

		return *this;
	}

	Framebuffer() {}

	~Framebuffer() {}
};


struct Pipeline {
	PipelineDesc  desc;


	Pipeline(const Pipeline &)            = default;
	Pipeline &operator=(const Pipeline &) = default;

	Pipeline(Pipeline &&other)
	: desc(other.desc)
	{
		other.desc = PipelineDesc();
	}

	Pipeline &operator=(Pipeline &&other) {
		if (this == &other) {
			return *this;
		}

		desc       = other.desc;

		other.desc = PipelineDesc();

		return *this;
	}

	Pipeline() {}

	~Pipeline() {}
};


struct RenderTarget {
	RenderTargetDesc  desc;


	RenderTarget(const RenderTarget &)            = default;
	RenderTarget &operator=(const RenderTarget &) = default;

	RenderTarget(RenderTarget &&other)
	: desc(other.desc)
	{
		other.desc = RenderTargetDesc();
	}

	RenderTarget &operator=(RenderTarget &&other) {
		if (this == &other) {
			return *this;
		}

		desc       = other.desc;

		other.desc = RenderTargetDesc();

		return *this;
	}

	RenderTarget() {}

	~RenderTarget() {}
};


struct Sampler {
	SamplerDesc desc;


	Sampler(const Sampler &)            = default;
	Sampler &operator=(const Sampler &) = default;

	Sampler(Sampler &&other)
	: desc(other.desc)
	{
		other.desc = SamplerDesc();
	}

	Sampler &operator=(Sampler &&other) {
		if (this == &other) {
			return *this;
		}

		desc       = other.desc;

		other.desc = SamplerDesc();

		return *this;
	}

	Sampler() {}

	~Sampler() {}
};


struct Texture {
	TextureDesc desc;


	Texture(const Texture &)            = default;
	Texture &operator=(const Texture &) = default;

	Texture(Texture &&other)
	: desc(other.desc)
	{
		other.desc = TextureDesc();
	}

	Texture &operator=(Texture &&other)
	{
		if (this == &other) {
			return *this;
		}

		desc = other.desc;

		other.desc = TextureDesc();

		return *this;
	}

	Texture() {}

	~Texture() {}
};


struct RendererBase {
	std::vector<char> ringBuffer;
	ResourceContainer<Buffer>              buffers;
	ResourceContainer<DescriptorSetLayout>  dsLayouts;
	ResourceContainer<Framebuffer>         framebuffers;
	ResourceContainer<Pipeline>            pipelines;
	ResourceContainer<RenderTarget>        rendertargets;
	ResourceContainer<Sampler>             samplers;
	ResourceContainer<Texture>             textures;

	PipelineDesc  currentPipeline;

	unsigned int numBuffers;
	unsigned int numTextures;

	std::vector<BufferHandle> ephemeralBuffers;


	RendererBase();

	~RendererBase();
};


} // namespace renderer


#endif  // NULLRENDERER_H
