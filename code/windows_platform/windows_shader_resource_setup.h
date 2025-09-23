
ID3D11ShaderResourceView*
SetupShaderResourceView(ID3D11Device* D11Device, game_texture_buffer *TextureBuffer)
{
    ID3D11ShaderResourceView* TextureView; 
    {
        u32 TextureArraySize = TextureBuffer->TexturesLoaded;

        D3D11_TEXTURE2D_DESC Desc = {};
        Desc.Width = TextureBuffer->TextureWidth;
        Desc.Height = TextureBuffer->TextureHeight;
        Desc.MipLevels = 1;
        Desc.ArraySize = TextureArraySize;
        Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc = { 1, 0 };
        Desc.Usage = D3D11_USAGE_IMMUTABLE;
        Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA *TextureSubResources = 
            (D3D11_SUBRESOURCE_DATA *)VirtualAlloc(0, sizeof(D3D11_SUBRESOURCE_DATA)*TextureArraySize,
                                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

        for (u32 TextureIndex = 0;
             TextureIndex < TextureArraySize;
             TextureIndex++)
        {
            D3D11_SUBRESOURCE_DATA SubResourceData = {};
            game_texture *Texture = &TextureBuffer->Textures[TextureIndex];
            SubResourceData.pSysMem = Texture->Data;
            SubResourceData.SysMemPitch = Texture->Width * sizeof(u32);
            TextureSubResources[TextureIndex] = SubResourceData;
        }

        ID3D11Texture2D* Texture;
        HRESULT HR = D11Device->CreateTexture2D(&Desc, TextureSubResources, &Texture);
        AssertHR(HR);

        HR = D11Device->CreateShaderResourceView((ID3D11Resource*)Texture, NULL, &TextureView);
        AssertHR(HR);

        Texture->Release();
        VirtualFree(TextureSubResources, 0, MEM_RELEASE);
    }

    return (TextureView);
}
