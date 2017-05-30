#ifdef RENDERER_NULL

#include "RendererInternal.h"
#include "Utils.h"



RendererImpl::RendererImpl(const RendererDesc &desc)
: swapchainDesc(desc.swapchain)
, savePreprocessedShaders(false)
, numBuffers(0)
, numPipelines(0)
, numSamplers(0)
, numTextures(0)
, inRenderPass(false)
{

	SDL_Init(SDL_INIT_EVENTS);
}


RendererImpl::~RendererImpl() {
	SDL_Quit();
}


BufferHandle RendererImpl::createBuffer(uint32_t size, const void *contents) {
	assert(size != 0);
	assert(contents != nullptr);

	// TODO: check desc

	numBuffers++;
	return numBuffers;
}


BufferHandle RendererImpl::createEphemeralBuffer(uint32_t size, const void *contents) {
	assert(size != 0);
	assert(contents != nullptr);

	return 0;
}


FramebufferHandle RendererImpl::createFramebuffer(const FramebufferDesc & /* desc */) {
	return FramebufferHandle(0);
}


PipelineHandle RendererImpl::createPipeline(const PipelineDesc & /* desc */) {
	numPipelines++;
	return numPipelines;
}


RenderTargetHandle RendererImpl::createRenderTarget(const RenderTargetDesc &desc) {
	assert(desc.width_  > 0);
	assert(desc.height_ > 0);
	assert(desc.format_ != Invalid);

	return 0;
}


SamplerHandle RendererImpl::createSampler(const SamplerDesc & /* desc */) {
	// TODO: check desc

	numSamplers++;
	return numSamplers;
}


VertexShaderHandle RendererImpl::createVertexShader(const std::string & /* name */, const ShaderMacros & /* macros */) {
	return VertexShaderHandle (0);
}


FragmentShaderHandle RendererImpl::createFragmentShader(const std::string & /* name */, const ShaderMacros & /* macros */) {
	return FragmentShaderHandle (0);
}


TextureHandle RendererImpl::createTexture(const TextureDesc &desc) {
	assert(desc.width_   > 0);
	assert(desc.height_  > 0);
	assert(desc.numMips_ > 0);

	// TODO: check data

	numTextures++;
	return numTextures;
}


void RendererImpl::deleteBuffer(BufferHandle /* handle */) {
}


void RendererImpl::deleteFramebuffer(FramebufferHandle /* fbo */) {
}


void RendererImpl::deleteRenderTarget(RenderTargetHandle &) {
}


void RendererImpl::deleteSampler(SamplerHandle /* handle */) {
}


void RendererImpl::deleteTexture(TextureHandle /* handle */) {
}


void RendererImpl::recreateSwapchain(const SwapchainDesc & /* desc */) {
}


void RendererImpl::blitFBO(FramebufferHandle /* src */, FramebufferHandle /* dest */) {
}


void RendererImpl::beginFrame() {
}


void RendererImpl::presentFrame(FramebufferHandle /* fbo */) {
}


void RendererImpl::beginRenderPass(FramebufferHandle /* fbo */) {
	assert(!inRenderPass);
	inRenderPass = true;
}


void RendererImpl::endRenderPass() {
	assert(inRenderPass);
	inRenderPass = false;
}


void RendererImpl::bindFramebuffer(FramebufferHandle fbo) {
	assert(fbo.handle);
}


void RendererImpl::bindPipeline(PipelineHandle pipeline) {
	assert(pipeline != 0);
}


void RendererImpl::bindIndexBuffer(BufferHandle /* buffer */, bool /* bit16 */ ) {
}


void RendererImpl::bindVertexBuffer(unsigned int /* binding */, BufferHandle /* buffer */) {
}


void RendererImpl::bindTexture(unsigned int /* unit */, TextureHandle /* tex */, SamplerHandle /* sampler */) {
}


void RendererImpl::bindUniformBuffer(unsigned int /* index */, BufferHandle /* buffer */) {
}


void RendererImpl::bindStorageBuffer(unsigned int /* index */, BufferHandle /* buffer */) {
}


void RendererImpl::setViewport(unsigned int /* x */, unsigned int /* y */, unsigned int /* width */, unsigned int /* height */) {
}


void RendererImpl::setScissorRect(unsigned int /* x */, unsigned int /* y */, unsigned int /* width */, unsigned int /* height */) {
}


void RendererImpl::draw(unsigned int /* firstVertex */, unsigned int /* vertexCount */) {
	assert(inRenderPass);
}


void RendererImpl::drawIndexedInstanced(unsigned int /* vertexCount */, unsigned int /* instanceCount */) {
	assert(inRenderPass);
}


#endif //  RENDERER_NULL
