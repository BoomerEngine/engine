/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#ifdef RENDER_COMMAND_OPCODE

RENDER_COMMAND_OPCODE(Nop) // no operation, nothing happens, some ops are transformed to Nops after device pass
RENDER_COMMAND_OPCODE(Hello) // name the command buffer (debug)
RENDER_COMMAND_OPCODE(NewBuffer) // end of data in current buffer, pointer to first data in new one
RENDER_COMMAND_OPCODE(ChildBuffer) // inserted child command buffer

RENDER_COMMAND_OPCODE(AcquireOutput) // acquire back buffer surfaces for rendering into output window
RENDER_COMMAND_OPCODE(SwapOutput) // swap the previously acquired back buffer surfaces

RENDER_COMMAND_OPCODE(BeginBlock) // mark start of a debug block
RENDER_COMMAND_OPCODE(EndBlock) // mark end of a debug block
RENDER_COMMAND_OPCODE(TriggerCapture) // trigger a capture if render doc is connected

RENDER_COMMAND_OPCODE(BeginPass) // bind a set of render targets (frame buffer) and start rendering to them
RENDER_COMMAND_OPCODE(EndPass) // finish rendering to render targets, optionally resolve frame MSAA render targets to a NON-MSAA ones
RENDER_COMMAND_OPCODE(Resolve) // resolve MSAA texture into non-MSAA stuff

RENDER_COMMAND_OPCODE(ClearPassColor) // fill current color buffer (bound to pass)
RENDER_COMMAND_OPCODE(ClearPassDepthStencil) // fill current depth/stencil buffer (bound to pass)

RENDER_COMMAND_OPCODE(ClearBuffer) // clear any buffer with any data, NOTE: uses copy engine
RENDER_COMMAND_OPCODE(ClearImage) // clear any image with any data, NOTE: uses copy engine

RENDER_COMMAND_OPCODE(BindParameters) // bind set of constant parameter
RENDER_COMMAND_OPCODE(BindVertexBuffer) // bind vertex data at from specified slot
RENDER_COMMAND_OPCODE(BindIndexBuffer) // bind index data

RENDER_COMMAND_OPCODE(UploadConstants) // upload constant data
RENDER_COMMAND_OPCODE(UploadParameters) // upload rendering parameters

RENDER_COMMAND_OPCODE(Draw) // draw non-indexed geometry
RENDER_COMMAND_OPCODE(DrawIndexed) // draw indexed geometry
RENDER_COMMAND_OPCODE(Dispatch) // dispatch a compute shader

RENDER_COMMAND_OPCODE(ImageLayoutBarrier) // create an image layout transition barrier
RENDER_COMMAND_OPCODE(GraphicsBarrier) // create a memory barrier in the graphics pipeline

RENDER_COMMAND_OPCODE(UpdateDynamicImage) // update content of the dynamic image
RENDER_COMMAND_OPCODE(UpdateDynamicBuffer) // update content of the dynamic buffer
RENDER_COMMAND_OPCODE(CopyBuffer) // copy data between buffers

RENDER_COMMAND_OPCODE(SetScissorState) // enable/disable scissor
RENDER_COMMAND_OPCODE(SetScissorRect) // set scissor area
RENDER_COMMAND_OPCODE(SetViewportRect) // set viewport area
RENDER_COMMAND_OPCODE(SetViewportDepthRange) // set viewport Z range (min Z/max Z)

RENDER_COMMAND_OPCODE(SetBlendState) // change the blend state for given render target index
RENDER_COMMAND_OPCODE(SetBlendColor) // set values for the const blending color

RENDER_COMMAND_OPCODE(SetColorMask) // change the color mask for given render target index
RENDER_COMMAND_OPCODE(SetCullState) // change the culling state
RENDER_COMMAND_OPCODE(SetFillState) // change the fill state

RENDER_COMMAND_OPCODE(SetDepthState) // change the depth state
RENDER_COMMAND_OPCODE(SetDepthBiasState) // change the depth bias state
RENDER_COMMAND_OPCODE(SetDepthClipState) // change the depth min/max ranges

RENDER_COMMAND_OPCODE(SetStencilState) // change the depth state
RENDER_COMMAND_OPCODE(SetStencilReference) // set values for the stencil reference value (requires the pipeline to allow for dynamic stencil reference value)
RENDER_COMMAND_OPCODE(SetStencilCompareMask) // set values for the stencil compare mask (requires the pipeline to allow for dynamic stencil compare mask)
RENDER_COMMAND_OPCODE(SetStencilWriteMask) // set values for the stencil write mask (requires the pipeline to allow for dynamic stencil write mask)

RENDER_COMMAND_OPCODE(SetPrimitiveAssemblyState) // change the primitive assembly state
RENDER_COMMAND_OPCODE(SetMultisampleState) // change the multisample state

RENDER_COMMAND_OPCODE(SignalCounter) // signal a job fence once execution gets to this place in command buffer - primary use is for data synchronization
RENDER_COMMAND_OPCODE(WaitForCounter) // wait for a counter before recording following commands to final form - primary use is for data synchronization

RENDER_COMMAND_OPCODE(DownloadBuffer) // download data from gpu-side buffer back to the CPU side
RENDER_COMMAND_OPCODE(DownloadImage) // download image from gpu-side buffer back to the CPUI side

#endif