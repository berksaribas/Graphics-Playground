#pragma once
#include "d3d_utils.h"
#include "maths.h"

struct Material {
	int type; // 0->emissive, 1->lambertian, 2->metal
	Vector3 albedo;
	float fuzziness;
};

struct RaytracerProperties {
	int width;
	int height;
	int frame_count;
	int sphere_count;
	Camera camera;
};

struct RaytracerData {
	RaytracerProperties properties;
	Sphere* spheres;
	Material* materials;
};

struct ComputeShaderData {
	ID3DBlob* cs_blob;
	ID3D11ComputeShader* compute_shader;
	ID3D11UnorderedAccessView* output_texture_view;
	ID3D11ShaderResourceView* output_texture_shader_view;
	ID3D11Texture2D* output_texture;

	StructuredDataBuffer properties;
	StructuredDataBuffer spheres;
	StructuredDataBuffer materials;
};

ComputeShaderData create_raytracer_shader(ID3D11Device* device, IDXGISwapChain* swapchain)
{
	ComputeShaderData data;

	data.cs_blob = nullptr;
	HRESULT hr = compile_shader(L"data/shaders/raytracer_compute.hlsl", "CS", "cs_5_0", device, &data.cs_blob);

	if (FAILED(hr))
	{
		printf("Failed compiling shader %08X\n", hr);
	}

	hr = device->CreateComputeShader(data.cs_blob->GetBufferPointer(), data.cs_blob->GetBufferSize(), nullptr, &data.compute_shader);

	if (FAILED(hr))
	{
		printf("Failed creating shader %08X\n", hr);
	}


	ID3D11Texture2D* back_buffer = nullptr;
	D3D11_TEXTURE2D_DESC back_buffer_desc = {};
	swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&back_buffer);
	back_buffer->GetDesc(&back_buffer_desc);

	D3D11_TEXTURE2D_DESC desc;
	desc.Width = back_buffer_desc.Width;
	desc.Height = back_buffer_desc.Height;
	printf("%d - %d\n", desc.Width, desc.Height);
	desc.MipLevels = desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	hr = device->CreateTexture2D(&desc, NULL, &data.output_texture);
	assert(SUCCEEDED(hr));

	hr = device->CreateShaderResourceView(data.output_texture, nullptr, &data.output_texture_shader_view);
	assert(SUCCEEDED(hr));

	//Create the UAV:
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.Flags = 0;
	uavDesc.Buffer.NumElements = desc.ArraySize;
	uavDesc.Format = desc.Format;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;

	hr = device->CreateUnorderedAccessView(data.output_texture, &uavDesc, &data.output_texture_view);
	assert(SUCCEEDED(hr));

	data.properties = create_structured_data_buffer(device, 1, sizeof(RaytracerProperties));
	data.spheres = create_structured_data_buffer(device, 100, sizeof(Sphere));
	data.materials = create_structured_data_buffer(device, 100, sizeof(Material));

	return data;
}

void raytracer_render(ID3D11DeviceContext* device_context, ComputeShaderData compute_data, RaytracerData& raytracer_data, QuadRenderer quad_renderer) {

	device_context->UpdateSubresource(compute_data.properties.buffer, 0, NULL, &raytracer_data.properties, 0, 0);
	device_context->UpdateSubresource(compute_data.spheres.buffer, 0, NULL, raytracer_data.spheres, 0, 0);
	device_context->UpdateSubresource(compute_data.materials.buffer, 0, NULL, raytracer_data.materials, 0, 0);

	ID3D11ShaderResourceView* shader_resource_views[] = {
		compute_data.properties.shader_resource_view,
		compute_data.spheres.shader_resource_view,
		compute_data.materials.shader_resource_view
	};
	device_context->CSSetShaderResources(0, ARRAYSIZE(shader_resource_views), shader_resource_views);
	device_context->CSSetShader(compute_data.compute_shader, nullptr, 0);
	UINT uavInitialCount = 0;
	device_context->CSSetUnorderedAccessViews(0, 1, &compute_data.output_texture_view, &uavInitialCount);
	device_context->Dispatch(raytracer_data.properties.width / 8, raytracer_data.properties.height / 8, 1);

	draw_quad(quad_renderer, device_context, compute_data.output_texture_shader_view);

	raytracer_data.properties.frame_count++;
}