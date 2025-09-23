

ID3D11Buffer *
SetupInstanceBuffer(ID3D11Device* D11Device, u32 InstanceCount, texture_draw_command_instance_buffer *GameInstanceBuffer)
{
    u32 InstanceBufferSize = sizeof(renderer_instance)*InstanceCount;
    GameInstanceBuffer->Size = InstanceBufferSize;
    GameInstanceBuffer->InstanceMax = InstanceCount;
    GameInstanceBuffer->Instances = 
        (renderer_instance *)VirtualAlloc(0, InstanceBufferSize, 
                                          MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    ID3D11Buffer *WindowsInstanceBuffer;
    {
        D3D11_BUFFER_DESC InstanceBufferDesc = {};
        InstanceBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        InstanceBufferDesc.ByteWidth = InstanceBufferSize;
        InstanceBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        InstanceBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        D3D11_SUBRESOURCE_DATA Initial = {};
        Initial.pSysMem = GameInstanceBuffer->Instances;
        HRESULT HR = D11Device->CreateBuffer(&InstanceBufferDesc, &Initial, &WindowsInstanceBuffer);
        AssertHR(HR);
    }

    return WindowsInstanceBuffer;
}
