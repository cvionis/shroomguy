#pragma once

#define G_ASSET_SLOTS_MAX 64

struct G_Sprite {
  RectF32 uv_rect;
  R_Texture *texture;
};

struct G_SpriteSheet {
  S32 width;
  S32 height;
  
  G_Sprite *sprites;
  U32 count;
};

enum G_AssetKind {
  G_AssetKind_Sprite,
  G_AssetKind_SpriteSheet,
};

struct G_Asset {
  String8 name;
  String8 path;
  
  U32 hash;
  
  B32 dirty;
  
  G_AssetKind kind;
  union {
    G_Sprite sprite;
    G_SpriteSheet spritesheet;
  };
};

struct G_AssetNode {
  G_AssetNode *list_next;
  G_AssetNode *hash_next;
  
  G_Asset v;
};

struct G_AssetSlot {
  G_AssetNode *first;
  G_AssetNode *last;
};

struct G_AssetContext {
  Arena *arena;
  
  G_AssetSlot slots[G_ASSET_SLOTS_MAX];
  G_AssetNode *first;
  G_AssetNode *last;
  U32 assets_count;
  
  String8 directory;
};

//
// Internal helpers
//

function void g_assets_sprite_load_impl(G_Sprite *sprite, String8 path);
function void g_assets_spritesheet_load_impl(Arena *arena, G_SpriteSheet *sheet, String8 path);

function G_Asset *g_assets_alloc_asset(G_AssetContext *ctx, String8 name);
function G_Sprite *g_assets_sprite_load(G_AssetContext *ctx, String8 name);
function G_SpriteSheet *g_assets_spritesheet_load(G_AssetContext *ctx, String8 name);

//
// Asset context
//

function G_AssetContext *g_assets_alloc(String8 directory);
function void g_assets_release(G_AssetContext *ctx);

//
// Asset retrievers 
//

function G_Asset *g_assets_get(G_AssetContext *ctx, String8 name);
function G_Sprite *g_assets_get_sprite(G_AssetContext *ctx, String8 name);
function G_SpriteSheet *g_assets_get_spritesheet(G_AssetContext *ctx, String8 name);

/*
TODO: Define spritesheet file format header and entry structs

Binary Sprite Sheet Format 
NOTE: sprites' image data are in row layout and are not packed
--------------------------------------------------------------

Header:
-------
 U32 frame_count      // Number of frames
 U16 sheet_width      // Total width of the sprite sheet (pixels)
 U16 sheet_height     // Height of the sprite sheet (pixels)

Per-Frame Metadata (repeated `frame_count` times):
------------------------------------------------
 U16 x                // X position of the frame in the sheet (in pixels)
 U16 y                // Y position of the frame in the sheet (always 0 in row layout)
 U16 width            // Width of the frame (pixels)
 U16 height           // Height of the frame (pixels)
 U32 duration_ms      // Frame duration in milliseconds

Image Data:
----------------------------
RGBA data for the entire sprite sheet
- Format: 4 bytes per pixel (RGBA8)
- Total size: sheet_width * sheet_height * sizeof(U32)

*/