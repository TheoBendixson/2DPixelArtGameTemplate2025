
internal void
CopyD3D11ShaderConstantsToGPU(ID3D11DeviceContext* DeviceContext, ID3D11Buffer* D3D11ConstantsBuffer,
                              r32 WindowWidth, r32 WindowHeight, v2 TextureSize)
{
    float RenderPassConstants[] = { WindowWidth, WindowHeight,
                                    TextureSize.X, TextureSize.Y };
    D3D11_MAPPED_SUBRESOURCE Mapped;
    HRESULT HR = DeviceContext->Map((ID3D11Resource*)D3D11ConstantsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    AssertHR(HR);
    memcpy(Mapped.pData, RenderPassConstants, sizeof(RenderPassConstants));
    DeviceContext->Unmap((ID3D11Resource*)D3D11ConstantsBuffer, 0);
}

internal void
CopyVertexBuffer(ID3D11DeviceContext* DeviceContext, ID3D11Buffer* WindowsVertexBuffer, 
                 game_texture_vertex_buffer *VertexBuffer)
{
    D3D11_MAPPED_SUBRESOURCE Mapped;
    HRESULT HR = DeviceContext->Map((ID3D11Resource*)WindowsVertexBuffer, 0, 
                                     D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    AssertHR(HR);
    memcpy(Mapped.pData, VertexBuffer->Vertices, VertexBuffer->Size);
    DeviceContext->Unmap((ID3D11Resource*)WindowsVertexBuffer, 0);
}

void
TransferInstanceBufferToGPU(ID3D11DeviceContext* DeviceContext, 
                            ID3D11Buffer *WindowsInstanceBuffer, 
                            texture_draw_command_instance_buffer *GameInstanceBuffer)
{
    D3D11_MAPPED_SUBRESOURCE Mapped;
    HRESULT HR = DeviceContext->Map((ID3D11Resource*)WindowsInstanceBuffer, 0, 
                                     D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    AssertHR(HR);
    memcpy(Mapped.pData, GameInstanceBuffer->Instances, GameInstanceBuffer->Size);
    DeviceContext->Unmap((ID3D11Resource*)WindowsInstanceBuffer, 0);
}

void
TransferTextureSizeToGPU(ID3D11DeviceContext* DeviceContext, ID3D11Buffer* TextureSizeConstantBuffer,
                         v2 TextureSize)
{
    float Constants[] = { TextureSize.X, TextureSize.Y };
    D3D11_MAPPED_SUBRESOURCE Mapped;
    HRESULT HR = DeviceContext->Map((ID3D11Resource*)TextureSizeConstantBuffer, 
                                     0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
    AssertHR(HR);
    memcpy(Mapped.pData, Constants, sizeof(Constants));
    DeviceContext->Unmap((ID3D11Resource*)TextureSizeConstantBuffer, 0);
}
