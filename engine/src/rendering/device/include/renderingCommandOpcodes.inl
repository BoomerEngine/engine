/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: interface\microps #]
***/

#ifdef RENDER_COMMAND_OPCODE

#undef CopyImage

// -- SYSTEM

RENDER_COMMAND_OPCODE(Nop) // no operation, nothing happens, some ops are transformed to Nops after device pass
RENDER_COMMAND_OPCODE(Hello) // name the command buffer (debug)
RENDER_COMMAND_OPCODE(NewBuffer) // end of data in current buffer, pointer to first data in new one
RENDER_COMMAND_OPCODE(ChildBuffer) // inserted child command buffer

RENDER_COMMAND_OPCODE(BeginBlock) // mark start of a debug block
RENDER_COMMAND_OPCODE(EndBlock) // mark end of a debug block
RENDER_COMMAND_OPCODE(TriggerCapture) // trigger a capture if render doc is connected

// -- OUTPUT/SWAPCHAIN

RENDER_COMMAND_OPCODE(AcquireOutput) // acquire back buffer surfaces for rendering into output window
RENDER_COMMAND_OPCODE(SwapOutput) // swap the previously acquired back buffer surfaces

// -- PASS

RENDER_COMMAND_OPCODE(BeginPass) // bind a set of render targets (frame buffer) and start rendering to them
RENDER_COMMAND_OPCODE(EndPass) // finish rendering to render targets, optionally resolve frame MSAA render targets to a NON-MSAA ones
RENDER_COMMAND_OPCODE(Resolve) // resolve MSAA texture into non-MSAA stuff

RENDER_COMMAND_OPCODE(ClearPassRenderTarget) // fill current color buffer (bound to pass)
RENDER_COMMAND_OPCODE(ClearPassDepthStencil) // fill current depth/stencil buffer (bound to pass)

RENDER_COMMAND_OPCODE(ClearRenderTarget) // fill current color buffer (bound to pass)
RENDER_COMMAND_OPCODE(ClearDepthStencil) // fill current depth/stencil buffer (bound to pass)

// -- RESOURCES

RENDER_COMMAND_OPCODE(ClearImage) // clear any image with any data
RENDER_COMMAND_OPCODE(ClearBuffer) // clear any buffer with any data
RENDER_COMMAND_OPCODE(Download) // download data from GPU resource back to the CPU side
RENDER_COMMAND_OPCODE(Update) // update content of the dynamic resource (image, buffer mostly)
RENDER_COMMAND_OPCODE(Copy) // copy data between buffers and images

// -- DRAWING

RENDER_COMMAND_OPCODE(BindDescriptor) // bind set of constant parameter
RENDER_COMMAND_OPCODE(BindVertexBuffer) // bind vertex data at from specified slot
RENDER_COMMAND_OPCODE(BindIndexBuffer) // bind index data

RENDER_COMMAND_OPCODE(UploadConstants) // upload constant data
RENDER_COMMAND_OPCODE(UploadDescriptor) // upload rendering parameters

RENDER_COMMAND_OPCODE(Draw) // draw non-indexed geometry
RENDER_COMMAND_OPCODE(DrawIndexed) // draw indexed geometry
RENDER_COMMAND_OPCODE(Dispatch) // dispatch a compute shader

RENDER_COMMAND_OPCODE(DrawIndirect) // draw non-indexed geometry indirectly
RENDER_COMMAND_OPCODE(DrawIndexedIndirect) // draw indexed geometry indirectly
RENDER_COMMAND_OPCODE(DispatchIndirect) // dispatch a compute shader indirectly

// -- SYNCHRONIZATION

RENDER_COMMAND_OPCODE(ResourceLayoutBarrier) // create an resource layout transition barrier
RENDER_COMMAND_OPCODE(UAVBarrier)

//-- DYNAMIC STATE

RENDER_COMMAND_OPCODE(SetLineWidth) // line width for line drawing mode
RENDER_COMMAND_OPCODE(SetScissorRect) // set scissor area
RENDER_COMMAND_OPCODE(SetViewportRect) // set viewport area + depth range
RENDER_COMMAND_OPCODE(SetBlendColor) // set values for the const blending color
RENDER_COMMAND_OPCODE(SetDepthClip) // change the depth min/max ranges
RENDER_COMMAND_OPCODE(SetStencilReference) // set values for the stencil reference value (requires the pipeline to allow for dynamic stencil reference value)

#endif