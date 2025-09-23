// Mooselutions asset packer..

#include "../game_library/mooselutions_asset_pack.h"
#include "../game_library/game_sound.h"

internal void
FillSoundAssetData(void* SoundFileContents, asset_header_content_data* ContentHeaderData, u32 ContentOffset)
{
    wav_file_header *WaveHeader = (wav_file_header *)SoundFileContents;
    ContentHeaderData->ContentSize = WaveHeader->DataSize;
    ContentHeaderData->ContentOffset = ContentOffset;
}

internal u32
GetEndOfContent(asset_header_content_data *AssetHeaderContent)
{
    return (AssetHeaderContent->ContentOffset + AssetHeaderContent->ContentSize);
}

internal void
CopyWavToPackFile(u8* DestFileData, void *SourceFileData, asset_header_content_data HeaderContentData)
{
    u8 *Dest = DestFileData + HeaderContentData.ContentOffset;
    void *Src = (u8 *)SourceFileData + sizeof(wav_file_header);
    memcpy(Dest, Src, HeaderContentData.ContentSize);
}

internal void
CopyRawAssetToPackFile(u8 *DestFileData, void *SourceFileData, asset_header_content_data* HeaderData)
{
    u8 *Dest = DestFileData + HeaderData->ContentOffset;
    void *Src = (u8 *)SourceFileData;
    memcpy(Dest, Src, HeaderData->ContentSize);
}

internal void
PackGameAssetsAndSaveToFile()
{
    read_file_result LoadedSpritesheetFiles[SPRITE_SHEET_COUNT];
    char *Filenames[SPRITE_SHEET_COUNT];

    Filenames[TILE_PACK_40_INDEX] = "art/TilePack40.png";
    Filenames[TILE_PACK_20_40_INDEX] = "art/TilePack_20_40.png";
    Filenames[TREE_PACK_INDEX] = "art/TreePack.png";
    Filenames[WORLD_MAP_PACK_INDEX] = "art/TilePack10.png";

    for (u32 FilenameIndex = 0;
         FilenameIndex < SPRITE_SHEET_COUNT;
         FilenameIndex++)
    {
        read_file_result FileReadResult = PlatformReadPNGFile(Filenames[FilenameIndex]); 

        if (FileReadResult.ContentsSize > 0)
        {
            LoadedSpritesheetFiles[FilenameIndex] = FileReadResult; 
        } else
        {
            PlatformTerminateApp();
        }
    }

    b32 SaveSpritesSucceeded = false;

    game_sprite_asset_pack_header AssetPackHeader = {};

    asset_header_content_data *PreviousHeaderData = NULL;

    for (u32 ContentIndex = 0;
         ContentIndex < SPRITE_SHEET_COUNT;
         ContentIndex++)
    {
        asset_header_content_data *HeaderData = &AssetPackHeader.HeaderContent[ContentIndex];
        read_file_result SpritesheetFile = LoadedSpritesheetFiles[ContentIndex];
        HeaderData->ContentSize = SpritesheetFile.ContentsSize;

        if (ContentIndex == 0)
        {
            HeaderData->ContentOffset = sizeof(game_sprite_asset_pack_header);
        } else
        {
            HeaderData->ContentOffset = GetEndOfContent(PreviousHeaderData);
        }

        PreviousHeaderData = HeaderData;

        if (ContentIndex == (SPRITE_SHEET_COUNT - 1))
        {
            AssetPackHeader.FileSize = GetEndOfContent(HeaderData);
        }
    }

    u8 *FileData = (u8 *)malloc(AssetPackHeader.FileSize);

    for (u32 ContentIndex = 0;
         ContentIndex < SPRITE_SHEET_COUNT;
         ContentIndex++)
    {
        read_file_result SpritesheetFile = LoadedSpritesheetFiles[ContentIndex];
        asset_header_content_data *HeaderData = &AssetPackHeader.HeaderContent[ContentIndex];
        CopyRawAssetToPackFile(FileData, SpritesheetFile.Contents, HeaderData);
    }

    memcpy(FileData, &AssetPackHeader, sizeof(game_sprite_asset_pack_header));

    b32 Written = PlatformWriteEntireFile("walod.summ", AssetPackHeader.FileSize, FileData);

    if (!Written)
    {
        PlatformTerminateApp();
    }
}
