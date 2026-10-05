// Private implementation included inside VulkanOverlay.cpp's anonymous namespace.
// Graphics passes work on the same queue as ImGui, including graphics-only queues.
// Cached resources are destroyed only after that queue goes idle.
struct BlurLevel {
    VkImage image{};
    VkDeviceMemory memory{};
    VkExtent2D extent{};
    VkImageLayout layout=VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageView view{};
    VkFramebuffer framebuffer{};
    VkDescriptorSet source{};
};
struct BlurCache {
std::array<BlurLevel,4> blurLevels;
ui::blur::Refresh blurRefresh;
bool blurFailed=false;
VkDescriptorSet blurTexture{};
VkSampler blurSampler{};
VkDescriptorSetLayout blurSetLayout{};
VkDescriptorPool blurPool{};
VkRenderPass blurPass{};
VkPipelineLayout blurPipelineLayout{};
VkPipeline blurPipeline{};
VkShaderModule blurVertex{},blurFragment{};

void DestroyBlur() {
    if(blurTexture)ImGui_ImplVulkan_RemoveTexture(blurTexture);
    blurTexture={};
    if(blurPipeline)pDestroyPipeline(device,blurPipeline,nullptr);
    if(blurPipelineLayout)pDestroyPipelineLayout(device,blurPipelineLayout,nullptr);
    if(blurVertex)pDestroyShaderModule(device,blurVertex,nullptr);
    if(blurFragment)pDestroyShaderModule(device,blurFragment,nullptr);
    if(blurPool)pDestroyDescriptorPool(device,blurPool,nullptr);
    if(blurSetLayout)pDestroyDescriptorSetLayout(device,blurSetLayout,nullptr);
    if(blurSampler)pDestroySampler(device,blurSampler,nullptr);
    for(auto& level:blurLevels) {
        if(level.framebuffer)pDestroyFramebuffer(device,level.framebuffer,nullptr);
        if(level.view)pDestroyImageView(device,level.view,nullptr);
        if(level.image)pDestroyImage(device,level.image,nullptr);
        if(level.memory)pFreeMemory(device,level.memory,nullptr);
        level={};
    }
    if(blurPass)pDestroyRenderPass(device,blurPass,nullptr);
    blurPipeline={};blurPipelineLayout={};blurVertex={};blurFragment={};blurPool={};blurSetLayout={};blurSampler={};blurPass={};
    blurRefresh={};blurFailed=false;
}
VkFormat BlurSampleFormat(VkFormat format) {
    if(format==VK_FORMAT_B8G8R8A8_SRGB)return VK_FORMAT_B8G8R8A8_UNORM;
    if(format==VK_FORMAT_R8G8B8A8_SRGB)return VK_FORMAT_R8G8B8A8_UNORM;
    return format;
}
bool CreateBlur() {
    if(blurFailed)return false;
    if(blurPipeline)return true;
    if(!chain.blurSupported || !imguiReady)return false;
    constexpr auto filteredFormat=VK_FORMAT_R16G16B16A16_SFLOAT;
    VkFormatProperties sourceProps{},sampleProps{},targetProps{};
    pGetPhysicalDeviceFormatProperties(physical,chain.format,&sourceProps);
    pGetPhysicalDeviceFormatProperties(physical,BlurSampleFormat(chain.format),&sampleProps);
    pGetPhysicalDeviceFormatProperties(physical,filteredFormat,&targetProps);
    constexpr auto sampled=VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT|VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;
    if(!(sourceProps.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_SRC_BIT) ||
        !(sourceProps.optimalTilingFeatures&VK_FORMAT_FEATURE_BLIT_DST_BIT) ||
        (sampleProps.optimalTilingFeatures&sampled)!=sampled ||
        (targetProps.optimalTilingFeatures&(sampled|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))!=(sampled|VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)) {
        blurFailed=true;LogTo("Vulkan: Gaussian blur unsupported for swapchain format %d",chain.format);return false;
    }
    try {
        VkSamplerCreateInfo sampler{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
        sampler.magFilter=sampler.minFilter=VK_FILTER_LINEAR;
        sampler.mipmapMode=VK_SAMPLER_MIPMAP_MODE_NEAREST;
        sampler.addressModeU=sampler.addressModeV=sampler.addressModeW=VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        Require(pCreateSampler(device,&sampler,nullptr,&blurSampler));
        VkDescriptorSetLayoutBinding binding{0,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,1,VK_SHADER_STAGE_FRAGMENT_BIT,nullptr};
        VkDescriptorSetLayoutCreateInfo set{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};set.bindingCount=1;set.pBindings=&binding;
        Require(pCreateDescriptorSetLayout(device,&set,nullptr,&blurSetLayout));
        VkDescriptorPoolSize poolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,3};
        VkDescriptorPoolCreateInfo pool{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};pool.maxSets=3;pool.poolSizeCount=1;pool.pPoolSizes=&poolSize;
        Require(pCreateDescriptorPool(device,&pool,nullptr,&blurPool));
        VkAttachmentDescription attachment{};attachment.format=filteredFormat;attachment.samples=VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;attachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout=attachment.finalLayout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        VkAttachmentReference reference{0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        VkSubpassDescription subpass{};subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;subpass.colorAttachmentCount=1;subpass.pColorAttachments=&reference;
        VkRenderPassCreateInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};pass.attachmentCount=1;pass.pAttachments=&attachment;pass.subpassCount=1;pass.pSubpasses=&subpass;
        Require(pCreateRenderPass(device,&pass,nullptr,&blurPass));
        VkPhysicalDeviceMemoryProperties memory{};pGetPhysicalDeviceMemoryProperties(physical,&memory);
        for(size_t i=0;i<blurLevels.size();++i) {
            auto& level=blurLevels[i];
            level.extent=i?VkExtent2D{std::max(1u,chain.extent.width/2),std::max(1u,chain.extent.height/2)}:chain.extent;
            const auto format=i?filteredFormat:chain.format;
            VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};info.flags=i?0:VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            info.imageType=VK_IMAGE_TYPE_2D;info.format=format;info.extent={level.extent.width,level.extent.height,1};
            info.mipLevels=info.arrayLayers=1;info.samples=VK_SAMPLE_COUNT_1_BIT;info.tiling=VK_IMAGE_TILING_OPTIMAL;
            info.usage=VK_IMAGE_USAGE_SAMPLED_BIT|(i?VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            info.sharingMode=VK_SHARING_MODE_EXCLUSIVE;Require(pCreateImage(device,&info,nullptr,&level.image));
            VkMemoryRequirements requirements{};pGetImageMemoryRequirements(device,level.image,&requirements);
            uint32_t type=memory.memoryTypeCount;
            for(uint32_t j=0;j<memory.memoryTypeCount;++j)if((requirements.memoryTypeBits&(1u<<j)) &&
                (memory.memoryTypes[j].propertyFlags&VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)){type=j;break;}
            if(type==memory.memoryTypeCount)throw std::runtime_error("No Gaussian image memory");
            VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};alloc.allocationSize=requirements.size;alloc.memoryTypeIndex=type;
            Require(pAllocateMemory(device,&alloc,nullptr,&level.memory));Require(pBindImageMemory(device,level.image,level.memory,0));
            VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};view.image=level.image;view.viewType=VK_IMAGE_VIEW_TYPE_2D;
            view.format=i?format:BlurSampleFormat(format);view.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
            Require(pCreateImageView(device,&view,nullptr,&level.view));
            if(i) {
                VkFramebufferCreateInfo fb{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};fb.renderPass=blurPass;fb.attachmentCount=1;fb.pAttachments=&level.view;
                fb.width=level.extent.width;fb.height=level.extent.height;fb.layers=1;Require(pCreateFramebuffer(device,&fb,nullptr,&level.framebuffer));
            }
            if(i<3) {
                VkDescriptorSetAllocateInfo allocation{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};allocation.descriptorPool=blurPool;
                allocation.descriptorSetCount=1;allocation.pSetLayouts=&blurSetLayout;Require(pAllocateDescriptorSets(device,&allocation,&level.source));
                VkDescriptorImageInfo image{blurSampler,level.view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
                VkWriteDescriptorSet write{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};write.dstSet=level.source;write.descriptorCount=1;
                write.descriptorType=VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;write.pImageInfo=&image;pUpdateDescriptorSets(device,1,&write,0,nullptr);
            }
        }
        VkPushConstantRange push{VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(ui::blur::GaussianPass)};
        VkPipelineLayoutCreateInfo layout{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};layout.setLayoutCount=1;layout.pSetLayouts=&blurSetLayout;
        layout.pushConstantRangeCount=1;layout.pPushConstantRanges=&push;Require(pCreatePipelineLayout(device,&layout,nullptr,&blurPipelineLayout));
        auto shader=[&](const auto& words,VkShaderModule& out){VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
            info.codeSize=sizeof(words);info.pCode=words;Require(pCreateShaderModule(device,&info,nullptr,&out));};
        shader(GaussianVertex,blurVertex);shader(GaussianFragment,blurFragment);
        VkPipelineShaderStageCreateInfo stages[2]{};
        for(auto& stage:stages){stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;stage.pName="main";}
        stages[0].stage=VK_SHADER_STAGE_VERTEX_BIT;stages[0].module=blurVertex;
        stages[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT;stages[1].module=blurFragment;
        VkPipelineVertexInputStateCreateInfo vertices{VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
        VkPipelineInputAssemblyStateCreateInfo assembly{VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};assembly.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPipelineViewportStateCreateInfo viewport{VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};viewport.viewportCount=viewport.scissorCount=1;
        VkPipelineRasterizationStateCreateInfo raster{VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};raster.polygonMode=VK_POLYGON_MODE_FILL;raster.lineWidth=1;
        VkPipelineMultisampleStateCreateInfo samples{VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};samples.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
        VkPipelineColorBlendAttachmentState blend{};blend.colorWriteMask=0xf;
        VkPipelineColorBlendStateCreateInfo colors{VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};colors.attachmentCount=1;colors.pAttachments=&blend;
        VkDynamicState dynamics[]{VK_DYNAMIC_STATE_VIEWPORT,VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};dynamic.dynamicStateCount=2;dynamic.pDynamicStates=dynamics;
        VkGraphicsPipelineCreateInfo pipeline{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};pipeline.stageCount=2;pipeline.pStages=stages;
        pipeline.pVertexInputState=&vertices;pipeline.pInputAssemblyState=&assembly;pipeline.pViewportState=&viewport;
        pipeline.pRasterizationState=&raster;pipeline.pMultisampleState=&samples;pipeline.pColorBlendState=&colors;pipeline.pDynamicState=&dynamic;
        pipeline.layout=blurPipelineLayout;pipeline.renderPass=blurPass;Require(pCreateGraphicsPipelines(device,{},1,&pipeline,nullptr,&blurPipeline));
        blurTexture=ImGui_ImplVulkan_AddTexture(blurLevels[3].view,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        if(!blurTexture)throw std::runtime_error("No Gaussian backdrop descriptor");
    }catch(const std::exception& error) {
        DestroyBlur();blurFailed=true;LogTo("Vulkan: Gaussian blur disabled: %s",error.what());return false;
    }
    return true;
}
void BlurBarrier(VkCommandBuffer command,VkImage image,VkImageLayout oldLayout,VkImageLayout next) {
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};barrier.oldLayout=oldLayout;barrier.newLayout=next;
    barrier.srcQueueFamilyIndex=barrier.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;barrier.image=image;
    barrier.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};
    barrier.srcAccessMask=oldLayout==VK_IMAGE_LAYOUT_UNDEFINED?0:VK_ACCESS_MEMORY_READ_BIT|VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask=next==VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL?VK_ACCESS_TRANSFER_READ_BIT:
        next==VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL?VK_ACCESS_TRANSFER_WRITE_BIT:
        next==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?VK_ACCESS_SHADER_READ_BIT:
        next==VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:0;
    pCmdPipelineBarrier(command,VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        next==VK_IMAGE_LAYOUT_PRESENT_SRC_KHR?VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT:
        next==VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL?VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT:
        next==VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL?VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT:VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,0,nullptr,0,nullptr,1,&barrier);
}
void BlurTransition(VkCommandBuffer command,BlurLevel& level,VkImageLayout layout) {
    BlurBarrier(command,level.image,level.layout,layout);level.layout=layout;
}
void GaussianFilter(VkCommandBuffer command,int from,int to,const ui::blur::GaussianPass& parameters) {
    auto& input=blurLevels[from];auto& output=blurLevels[to];
    BlurTransition(command,input,VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    BlurTransition(command,output,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderPassBeginInfo begin{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};begin.renderPass=blurPass;begin.framebuffer=output.framebuffer;begin.renderArea.extent=output.extent;
    pCmdBeginRenderPass(command,&begin,VK_SUBPASS_CONTENTS_INLINE);
    pCmdBindPipeline(command,VK_PIPELINE_BIND_POINT_GRAPHICS,blurPipeline);
    pCmdBindDescriptorSets(command,VK_PIPELINE_BIND_POINT_GRAPHICS,blurPipelineLayout,0,1,&input.source,0,nullptr);
    pCmdPushConstants(command,blurPipelineLayout,VK_SHADER_STAGE_FRAGMENT_BIT,0,sizeof(parameters),&parameters);
    VkViewport viewport{0,0,float(output.extent.width),float(output.extent.height),0,1};VkRect2D scissor{{0,0},output.extent};
    pCmdSetViewport(command,0,1,&viewport);pCmdSetScissor(command,0,1,&scissor);pCmdDraw(command,3,1,0,0);pCmdEndRenderPass(command);
}
void RenderBlur(VkCommandBuffer command,VkImage image,ui::blur::Options options,bool force=false) {
    if(!options.enabled){blurRefresh.valid=false;return;}
    if(!CreateBlur())return;
    if(force || blurRefresh.Due(options)) {
        BlurBarrier(command,image,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        BlurTransition(command,blurLevels[0],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        VkImageBlit copy{};copy.srcSubresource=copy.dstSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};
        copy.srcOffsets[1]=copy.dstOffsets[1]={int32_t(chain.extent.width),int32_t(chain.extent.height),1};
        pCmdBlitImage(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,blurLevels[0].image,VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&copy,VK_FILTER_NEAREST);
        GaussianFilter(command,0,1,ui::blur::GaussianPass{});
        GaussianFilter(command,1,2,ui::blur::GaussianPass(options.strength,1.f/blurLevels[1].extent.width,0));
        GaussianFilter(command,2,3,ui::blur::GaussianPass(options.strength,0,1.f/blurLevels[1].extent.height));
        BlurTransition(command,blurLevels[3],VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        BlurBarrier(command,image,VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        blurRefresh.Updated(options);
    }
}
};
BlurCache worldBlur, layerBlur;
void DestroyBlur() {
    ui::blur::Publish(0);ui::blur::ClearLayerBackdrop();
    worldBlur.DestroyBlur();layerBlur.DestroyBlur();
}
void CaptureLayer(const ImDrawList*,const ImDrawCmd*) {
    if(!recording)return;
    // End/store the UI pass before sampling it, then resume with LOAD.
    pCmdEndRenderPass(recording);
    layerBlur.RenderBlur(recording,chain.images[recordingImageIndex].image,ui::blur::GetOptions(),true);
    VkRenderPassBeginInfo pass{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    pass.renderPass=chain.pass;pass.framebuffer=chain.images[recordingImageIndex].framebuffer;
    pass.renderArea.extent=chain.extent;
    pCmdBeginRenderPass(recording,&pass,VK_SUBPASS_CONTENTS_INLINE);
}
void RenderBlur(VkCommandBuffer command,VkImage image,ui::blur::Options options) {
    ui::blur::Publish(0);ui::blur::ClearLayerBackdrop();
    worldBlur.RenderBlur(command,image,options);
    if(!options.enabled)return;
    if(worldBlur.blurRefresh.valid)ui::blur::Publish(reinterpret_cast<ImTextureID>(worldBlur.blurTexture));
    if(layerBlur.CreateBlur())ui::blur::SetLayerBackdrop(reinterpret_cast<ImTextureID>(layerBlur.blurTexture),false,CaptureLayer);
}
