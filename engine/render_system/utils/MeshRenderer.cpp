#include "MeshRenderer.hpp"

#include "LogicalDevice.hpp"

namespace MFA
{

	//-------------------------------------------------------------------------------------------------

	MeshRenderer::MeshRenderer(
		std::shared_ptr<FlatShadingPipeline> pipeline,
		std::shared_ptr<AS::GLTF::Model> const& model,
		std::shared_ptr<RT::GpuTexture> errorTexture
	)
		: _pipeline(std::move(pipeline))
		, _meshData(model->mesh->GetMeshData())
		, _errorTexture(std::move(errorTexture))
	{

		if (model->mesh->IsCentered() == false)
		{
			model->mesh->CenterMesh();
		}

		const auto device = LogicalDevice::Instance;

		const auto commandBuffer = RB::BeginSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool()
		);

		auto vertexStageBuffer = GenerateVertexBuffer(commandBuffer, *model);

		auto indexStageBuffer = GenerateIndexBuffer(commandBuffer, *model);

		auto textureStageBuffer = GenerateTextures(commandBuffer, *model);

		auto materialStageBuffer = CreateMaterials(commandBuffer);

		CreateDescriptorSets();

		_vertexCount = model->mesh->GetVertexCount();
		_vertices = model->mesh->GetVertexData();

		_indexCount = model->mesh->GetIndexCount();
		_indices = model->mesh->GetIndexData();

		GenerateCollisionTriangles(*model);

		RB::EndAndSubmitSingleTimeCommand(
			device->GetVkDevice(),
			device->GetGraphicCommandPool(),
			device->GetGraphicQueue(),
			commandBuffer
		);
	}

	//-------------------------------------------------------------------------------------------------

	void MeshRenderer::Render(RT::CommandRecordState& recordState, std::vector<glm::mat4> const& models) const
	{
		_pipeline->BindPipeline(recordState);

		RB::BindIndexBuffer(
			recordState,
			*_indicesBuffer,
			0,
			VK_INDEX_TYPE_UINT32
		);

		RB::BindVertexBuffer(
			recordState,
			*_verticesBuffer,
			0,
			0
		);
		for (auto const& model : models)
		{
			auto const & rootNodes = _meshData->rootNodes;
			auto const& nodes = _meshData->nodes;
			for (auto & rootNode : rootNodes)
			{
				auto const& node = nodes[rootNode];
				DrawNode(
					recordState, 
					node, 
					model
				);
			}
		}
	}

	//-------------------------------------------------------------------------------------------------

	std::shared_ptr<RT::BufferGroup> MeshRenderer::GenerateVertexBuffer(VkCommandBuffer cb, AS::GLTF::Model const& model)
	{
		auto& mesh = model.mesh;

		auto* gltfVertices = mesh->GetVertexData()->As<AS::GLTF::Vertex>();
		auto gltfVertexCount = mesh->GetVertexCount();

		std::vector<FlatShadingPipeline::Vertex> cpuVertices(gltfVertexCount);

		for (int i = 0; i < static_cast<int>(gltfVertexCount); ++i)
		{
			auto const& gltfVertex = gltfVertices[i];
			cpuVertices[i] = FlatShadingPipeline::Vertex
			{
				.position = gltfVertex.position,
				.baseColorUV = gltfVertex.baseColorUV,
				.normal = gltfVertex.normal
			};
		}

		auto alias = Alias(cpuVertices.data(), cpuVertices.size());

		auto* device = LogicalDevice::Instance;

		auto const stageBuffer = RB::CreateStageBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			alias.Len(),
			1
		);

		_verticesBuffer = RB::CreateVertexBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			cb,
			*stageBuffer->buffers[0],
			alias
		);

		return stageBuffer;
	}

	//-------------------------------------------------------------------------------------------------

	std::shared_ptr<RT::BufferGroup> MeshRenderer::GenerateIndexBuffer(
		VkCommandBuffer cb,
		AS::GLTF::Model const& model
	)
	{
		auto& mesh = model.mesh;

		auto const gltfIndices = mesh->GetIndexData();

		auto* device = LogicalDevice::Instance;

		auto const indexStageBuffer = RB::CreateStageBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			gltfIndices->Len(),
			1
		);

		_indicesBuffer = RB::CreateIndexBuffer(
			device->GetVkDevice(),
			device->GetPhysicalDevice(),
			cb,
			*indexStageBuffer->buffers[0],
			*gltfIndices
		);

		return indexStageBuffer;
	}

	//-------------------------------------------------------------------------------------------------

	std::vector<std::shared_ptr<RT::BufferAndMemory>> MeshRenderer::GenerateTextures(VkCommandBuffer cb,
		AS::GLTF::Model const& model)
	{
		std::vector<std::shared_ptr<RT::BufferAndMemory>> stagingBuffers{};

		_textures.clear();

		auto* device = LogicalDevice::Instance;

		for (auto& cpuTexture : model.textures)
		{
			auto [gpuTexture, stageBuffer] = RB::CreateTexture(
				*cpuTexture,
				device->GetVkDevice(),
				device->GetPhysicalDevice(),
				cb
			);
			_textures.emplace_back(gpuTexture);
			stagingBuffers.emplace_back(stageBuffer);
		}

		return stagingBuffers;
	}

	//-------------------------------------------------------------------------------------------------

	std::vector<std::shared_ptr<RT::BufferGroup>> MeshRenderer::CreateMaterials(VkCommandBuffer cb)
	{
		std::vector<std::shared_ptr<RT::BufferGroup>> stagingBuffers{};

		_materials.clear();

		auto* device = LogicalDevice::Instance;

		for (auto const& subMesh : _meshData->subMeshes)
		{
			for (auto const& primitive : subMesh.primitives)
			{
				FlatShadingPipeline::Material data{
					.color = glm::vec4{
						primitive.baseColorFactor[0],
						primitive.baseColorFactor[1],
						primitive.baseColorFactor[2],
						primitive.baseColorFactor[3]
					},
					.hasBaseColorTexture = primitive.hasBaseColorTexture ? 1 : 0
				};

				auto materialBuffer = RB::CreateLocalUniformBuffer(
					device->GetVkDevice(),
					device->GetPhysicalDevice(),
					sizeof(data),
					1
				);

				auto stageBuffer = RB::CreateStageBuffer(
					device->GetVkDevice(),
					device->GetPhysicalDevice(),
					sizeof(data),
					1
				);

				RB::UpdateHostVisibleBuffer(
					device->GetVkDevice(),
					*stageBuffer->buffers[0],
					Alias(data)
				);

				RB::UpdateLocalBuffer(
					cb,
					*materialBuffer->buffers[0],
					*stageBuffer->buffers[0]
				);

				stagingBuffers.emplace_back(stageBuffer);
				_materials.emplace_back(materialBuffer);
			}
		}

		return stagingBuffers;
	}

	//-------------------------------------------------------------------------------------------------

	void MeshRenderer::CreateDescriptorSets()
	{
		int nextMaterialIdx = 0;

		_descriptorSets.clear();

		for (auto const& subMesh : _meshData->subMeshes)
		{
			_descriptorSets.emplace_back();
			auto & descriptorSets = _descriptorSets.back();

			for (auto const& primitive : subMesh.primitives)
			{
				auto const* gpuTexture = _errorTexture.get();

				if (primitive.hasBaseColorTexture == true)
				{
					gpuTexture = _textures[primitive.baseColorTextureIndex].get();
				}

				descriptorSets.emplace_back(
					_pipeline->CreatePerGeometryDescriptorSetGroup(
						*_materials[nextMaterialIdx]->buffers[0],
						*gpuTexture
					)
				);

				++nextMaterialIdx;
			}
		}
	}

	//-------------------------------------------------------------------------------------------------

	void MeshRenderer::GenerateCollisionTriangles(AS::GLTF::Model const& model)
	{
		_collisionTriangles = {};

		auto const& mesh = model.mesh;

		auto const* indices = mesh->GetIndexData()->As<AS::GLTF::Index>();
		auto const indicesCount = mesh->GetIndexCount();

		auto const* vertices = mesh->GetVertexData()->As<AS::GLTF::Vertex>();

		MFA_ASSERT(indicesCount % 3 == 0);

		int faceCount = static_cast<int>(indicesCount) / 3;
		// TODO: This chunk of code is very useful. We need a helper function from it
		for (int i = 0; i < faceCount; ++i)
		{
			auto const idx0 = indices[3 * i];
			auto const idx1 = indices[3 * i + 1];
			auto const idx2 = indices[3 * i + 2];

			auto const& vertex0 = vertices[idx0];
			auto const& vertex1 = vertices[idx1];
			auto const& vertex2 = vertices[idx2];

			glm::dvec3 const v0Pos = vertex0.position;
			glm::dvec3 const v1Pos = vertex1.position;
			glm::dvec3 const v2Pos = vertex2.position;

			_collisionTriangles.emplace_back(Collision::GenerateCollisionTriangle(v0Pos, v1Pos, v2Pos));
		}
	}

	//-------------------------------------------------------------------------------------------------

	void MeshRenderer::DrawSubMesh(
		RT::CommandRecordState& recordState,
		int const subMeshIdx,
		glm::mat4 const& transform
	) const
	{
		auto const& subMesh = _meshData->subMeshes[subMeshIdx];
		auto const& descriptorSets = _descriptorSets[subMeshIdx];

		_pipeline->SetPushConstants(
			recordState,
			FlatShadingPipeline::PushConstants{
				.model = transform
			}
		);

		for (int i = 0; i < static_cast<int>(subMesh.primitives.size()); ++i)
		{
			auto const& primitive = subMesh.primitives[i];
			RB::AutoBindDescriptorSet(
				recordState,
				RB::UpdateFrequency::PerGeometry,
				descriptorSets[i].descriptorSets[0]
			);

			RB::DrawIndexed(
				recordState,
				primitive.indicesCount,
				1,
				primitive.indicesStartingIndex
			);
		}
	}

	//-------------------------------------------------------------------------------------------------

	void MeshRenderer::DrawNode(
		RT::CommandRecordState& recordState, 
		Asset::GLTF::Node const& node,
		glm::mat4 const& parentTransform
	) const
	{
		auto const transform = parentTransform * node.cacheTransform;
		if (node.hasSubMesh())
		{
			DrawSubMesh(recordState, node.subMeshIndex, transform);
		}

		for (auto const child : node.children)
		{
			DrawNode(recordState, _meshData->nodes[child], transform);
		}
	}

	//-------------------------------------------------------------------------------------------------

	[[nodiscard]]
	std::vector<CollisionTriangle> MeshRenderer::GetCollisionTriangles(glm::mat4 const& model) const noexcept
	{
		auto copy = _collisionTriangles;
		for (auto& triangle : copy)
		{
			triangle.normal = glm::normalize(model * glm::vec4{ triangle.normal, 0.0f });
			triangle.center = model * glm::vec4{ triangle.center, 1.0f };

			for (auto& edgeNormal : triangle.edgeNormals)
			{
				edgeNormal = glm::normalize(model * glm::vec4{ edgeNormal, 0.0f });
			}
			for (auto& edgeVertex : triangle.edgeVertices)
			{
				edgeVertex = model * glm::vec4{ edgeVertex, 1.0f };
			}
		}
		return copy;
	}

	//-------------------------------------------------------------------------------------------------

	std::vector<glm::vec3> MeshRenderer::GetVertices(glm::mat4 const& model) const noexcept
	{
		std::vector<glm::vec3> copy(_vertexCount);

		auto const* vertices = _vertices->As<AS::GLTF::Vertex>();
		for (int i = 0; i < _vertexCount; ++i)
		{
			copy[i] = model * glm::vec4{vertices[i].position, 1.0f};
		}

		return copy;
	}

	//-------------------------------------------------------------------------------------------------

	std::vector<std::vector<std::tuple<int, int>>> MeshRenderer::GetNeighbors() const noexcept
	{
		std::vector<std::vector<std::tuple<int, int>>> result{};

		auto const indices = _indices->As<AS::GLTF::Index>();
		
		int faceCount = static_cast<int>(_indexCount) / 3;

		result.resize(_vertexCount);

		// TODO: This chunk of code is very useful. We need a helper function from it
		for (int i = 0; i < faceCount; ++i)
		{
			auto const idx0 = indices[3 * i];
			auto const idx1 = indices[3 * i + 1];
			auto const idx2 = indices[3 * i + 2];

			result[idx0].emplace_back(std::tuple<int, int>{idx1, idx2});
			result[idx1].emplace_back(std::tuple<int, int>{idx2, idx0});
			result[idx2].emplace_back(std::tuple<int, int>{idx0, idx1});
		}

		return result;
	}

	//-------------------------------------------------------------------------------------------------

}
