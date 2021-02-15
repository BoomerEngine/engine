/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: command\tests #]
***/

#include "build.h"
#include "renderingTest.h"

#include "rendering/device/include/renderingDeviceApi.h"
#include "rendering/device/include/renderingCommandWriter.h"

namespace rendering
{
    namespace test
    {
        /// a basic test of inline parameters
        class RenderingTest_InlineParameters : public IRenderingTest
        {
            RTTI_DECLARE_VIRTUAL_CLASS(RenderingTest_InlineParameters, IRenderingTest);

        public:
            virtual void initialize() override final;
            virtual void render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView ) override final;

			virtual void describeSubtest(base::IFormatStream& f) override
			{
				if (subTestIndex() == 0)
					f.append("NoThreads");
				else
					f.appendf("Threads{}", 1 << subTestIndex());
			}

        private:
            BufferObjectPtr m_vertexBuffer;
            GraphicsPipelineObjectPtr m_shaders;

			void recordRowRange(command::CommandWriter& cmd, uint32_t firstRow, uint32_t lastRow);

			static const auto GRID_SIZE = 64;
        };

        RTTI_BEGIN_TYPE_CLASS(RenderingTest_InlineParameters);
			RTTI_METADATA(RenderingTestOrderMetadata).order(70);
			RTTI_METADATA(RenderingTestSubtestCountMetadata).count(5);
        RTTI_END_TYPE();

        //---       

        void RenderingTest_InlineParameters::initialize()
        {
            m_shaders = loadGraphicsShader("InlineParameters.csl");

            // create vertex buffer with a single triangle
            {
                Simple3DVertex vertices[9];
                vertices[0].set(0, -1, 0.5f, 0, 0, base::Color::WHITE);
                vertices[1].set(-1, 1, 0.5f, 0, 0, base::Color::WHITE);
                vertices[2].set(1, 1, 0.5f, 0, 0, base::Color::WHITE);
                m_vertexBuffer = createVertexBuffer(sizeof(vertices), vertices);
            }
        }

		void RenderingTest_InlineParameters::recordRowRange(command::CommandWriter& cmd, uint32_t firstRow, uint32_t lastRow)
		{
			//--

			struct InlineParameterConsts
			{
				base::Vector2 TestOffset = base::Vector2(0, 0);
				base::Vector2 TestScale = base::Vector2(1, 1);
			};

			struct InlineParameterConstsEx
			{
				base::Vector4 TestColor = base::Vector4(1, 1, 1, 1);
			};
			
			//--

			static const auto TestParams = "TestParams"_id;

			cmd.opBindVertexBuffer("Simple3DVertex"_id, m_vertexBuffer);

			const auto infGridSize = 1.0f / (GRID_SIZE - 1);
			for (uint32_t y = firstRow; y < lastRow; ++y)
			{
				float fracY = y * infGridSize;
				for (uint32_t x = 0; x < GRID_SIZE; ++x)
				{
					float fracX = x * infGridSize;

					InlineParameterConsts params;
					params.TestScale.x = infGridSize;
					params.TestScale.y = infGridSize;
					params.TestOffset.x = -1.0f + 2.0f * params.TestScale.x * (0.5f + x);
					params.TestOffset.y = -1.0f + 2.0f * params.TestScale.y * (0.5f + y);

					InlineParameterConstsEx paramsEx;
					paramsEx.TestColor.x = fracX;
					paramsEx.TestColor.y = fracY;
					paramsEx.TestColor.z = 0.0f;
					paramsEx.TestColor.w = 1.0f;

					DescriptorEntry desc[2];
					desc[0].constants(params);
					desc[1].constants(paramsEx);
					cmd.opBindDescriptor(TestParams, desc);

					cmd.opDraw(m_shaders, 0, 3);
				}
			}

			//--
		}

        void RenderingTest_InlineParameters::render(command::CommandWriter& cmd, float time, const RenderTargetView* backBufferView, const RenderTargetView* backBufferDepthView )
        {
			//--

            FrameBuffer fb;
            fb.color[0].view(backBufferView).clear(base::Vector4(0.0f, 0.0f, 0.2f, 1.0f));

			cmd.opBeingPass(fb);

			// draw using ST or MT approach
			if (subTestIndex() == 0)
			{
				recordRowRange(cmd, 0, GRID_SIZE);
			}
			else
			{
				auto numFibers = 1 << subTestIndex();
				auto counter = Fibers::GetInstance().createCounter("RecordFiber", numFibers);

				// spawn jobs
				for (uint32_t i = 0; i < numFibers; ++i)
				{
					auto childCommandBuffer = cmd.opCreateChildCommandBuffer();

					auto firstRow = (i * GRID_SIZE) / numFibers;
					auto lastRow = std::min<uint32_t>(GRID_SIZE, ((i + 1) * GRID_SIZE) / numFibers);

					RunChildFiber("RecordCommands") << [this, childCommandBuffer, firstRow, lastRow, counter](FIBER_FUNC)
					{
						command::CommandWriter childWriter(childCommandBuffer);
						recordRowRange(childWriter, firstRow, lastRow);
						Fibers::GetInstance().signalCounter(counter);
					};
				}

				// wait for all recording jobs to finish
				Fibers::GetInstance().waitForCounterAndRelease(counter);
			}           

            cmd.opEndPass();
        }

    } // test
} // rendering