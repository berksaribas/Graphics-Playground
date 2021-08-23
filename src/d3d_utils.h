#pragma once

#include <stdio.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <assert.h>

struct QuadRenderer {
    ID3D11InputLayout* input_layout;
    ID3D11VertexShader* vertex_shader;
    ID3D11PixelShader* pixel_shader;
    ID3D11SamplerState* sampler_state;
    ID3D11Buffer* vertex_buffer;
};

struct StructuredDataBuffer {
    ID3D11Buffer* buffer;
    ID3D11ShaderResourceView* shader_resource_view;
};

static HRESULT compile_shader(LPCWSTR src_file, LPCSTR entry_point, LPCSTR shader_type, ID3D11Device* device, ID3DBlob** blob)
{
    if (!src_file || !entry_point || !device || !blob)
    {
        return E_INVALIDARG;
    }

    *blob = nullptr;

    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#if defined( DEBUG ) || defined( _DEBUG )
    flags |= D3DCOMPILE_DEBUG;
#endif

    ID3DBlob* shaderBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    
    HRESULT hr = D3DCompileFromFile(src_file, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entry_point, shader_type,
        flags, 0, &shaderBlob, &errorBlob);
    
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }

        if (shaderBlob) {
            shaderBlob->Release();
        }

        return hr;
    }

    *blob = shaderBlob;

    return hr;
}

static StructuredDataBuffer create_structured_data_buffer(ID3D11Device* device, int count, int size) {
    StructuredDataBuffer structured_data_buffer;

    D3D11_BUFFER_DESC buffer_desc = {};
    buffer_desc.ByteWidth = count * size;
    buffer_desc.Usage = D3D11_USAGE_DEFAULT;
    buffer_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    buffer_desc.CPUAccessFlags = 0;
    buffer_desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    buffer_desc.StructureByteStride = size;
    HRESULT hr = device->CreateBuffer(&buffer_desc, NULL, &structured_data_buffer.buffer);
    assert(SUCCEEDED(hr));

    D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
    shader_resource_view_desc.Format = DXGI_FORMAT_UNKNOWN;
    shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    shader_resource_view_desc.Buffer.FirstElement = 0;
    shader_resource_view_desc.Buffer.NumElements = count;

    hr = device->CreateShaderResourceView(structured_data_buffer.buffer, &shader_resource_view_desc, &structured_data_buffer.shader_resource_view);
    assert(SUCCEEDED(hr));

    return structured_data_buffer;
}

static QuadRenderer initialize_quad_renderer(ID3D11Device* device) {
    QuadRenderer quad_renderer;

    ID3DBlob* vs_blob;
    ID3DBlob* ps_blob;
    {
        HRESULT hr = compile_shader(L"data/shaders/quad.hlsl", "vs_main", "vs_5_0", device, &vs_blob);
        assert(SUCCEEDED(hr));
        hr = compile_shader(L"data/shaders/quad.hlsl", "ps_main", "ps_5_0", device, &ps_blob);
        assert(SUCCEEDED(hr));
        hr = device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &quad_renderer.vertex_shader);
        assert(SUCCEEDED(hr));
        hr = device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &quad_renderer.pixel_shader);
        assert(SUCCEEDED(hr));
        ps_blob->Release();
    }

    {
        D3D11_INPUT_ELEMENT_DESC input_element_desc[] =
        {
            { "POS", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        HRESULT hResult = device->CreateInputLayout(input_element_desc, ARRAYSIZE(input_element_desc), vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &quad_renderer.input_layout);
        assert(SUCCEEDED(hResult));
        vs_blob->Release();
    }

    {
        float vertexData[] = { // x, y, u, v
            -1.0f,  1.0f, 0.f, 0.f,
            1.0f, -1.0f, 1.f, 1.f,
            -1.0f, -1.0f, 0.f, 1.f,
            -1.0f,  1.0, 0.f, 0.f,
            1.0f,  1.0f, 1.f, 0.f,
            1.0f, -1.0f, 1.f, 1.f
        };

        D3D11_BUFFER_DESC vertex_buffer_desc = {};
        vertex_buffer_desc.ByteWidth = sizeof(vertexData);
        vertex_buffer_desc.Usage = D3D11_USAGE_IMMUTABLE;
        vertex_buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        D3D11_SUBRESOURCE_DATA vertex_subresource_data = { vertexData };

        HRESULT hResult = device->CreateBuffer(&vertex_buffer_desc, &vertex_subresource_data, &quad_renderer.vertex_buffer);
        assert(SUCCEEDED(hResult));
    }

    D3D11_SAMPLER_DESC sampler_desc = {};
    sampler_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sampler_desc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
    sampler_desc.BorderColor[0] = 1.0f;
    sampler_desc.BorderColor[1] = 1.0f;
    sampler_desc.BorderColor[2] = 1.0f;
    sampler_desc.BorderColor[3] = 1.0f;
    sampler_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;

    device->CreateSamplerState(&sampler_desc, &quad_renderer.sampler_state);

    return quad_renderer;
}

static void draw_quad(QuadRenderer quad_renderer, ID3D11DeviceContext* device_context, ID3D11ShaderResourceView* texture_view) {
    UINT stride = 4 * sizeof(float);
    UINT vertices = 6;
    UINT offset = 0;

    device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    device_context->IASetInputLayout(quad_renderer.input_layout);

    device_context->VSSetShader(quad_renderer.vertex_shader, nullptr, 0);
    device_context->PSSetShader(quad_renderer.pixel_shader, nullptr, 0);

    ID3D11UnorderedAccessView* NullUav = nullptr;
    device_context->CSSetUnorderedAccessViews(0, 1, &NullUav, nullptr);

    device_context->PSSetShaderResources(0, 1, &texture_view);
    device_context->PSSetSamplers(0, 1, &quad_renderer.sampler_state);

    device_context->IASetVertexBuffers(0, 1, &quad_renderer.vertex_buffer, &stride, &offset);

    device_context->Draw(vertices, 0);
}