// 
// Internal helpers
//

function void
g_assets_sprite_load_impl(G_Sprite *sprite, String8 path)
{
  if (sprite) {
    if (sprite->texture) {
      r_texture_delete(&sprite->texture);
      sprite->texture = 0;
    }
    
    R_Texture *texture = r_texture_load(path, R_TextureFormat_RGBA8);
    
    sprite->texture = texture;
    sprite->uv_rect = rect_f32(0,0,1,1);
  }
}

function void
g_assets_spritesheet_load_impl(Arena *arena, G_SpriteSheet *sheet, String8 path)
{
  if (sheet) {
    TempArena scratch = arena_scratch_begin(0, 0);
    
    String8 file = os_file_read(scratch.arena, path);
    if (file.count) {
      U8 *pos = file.data;
      
      U32 spr_count = 0;
      MemoryCopy(&spr_count, pos, sizeof(U32));
      pos += sizeof(U32);
      
      U16 sheet_width = 0;
      MemoryCopy(&sheet_width, pos, sizeof(U16));
      pos += sizeof(U16);
      
      U16 sheet_height = 0;
      MemoryCopy(&sheet_height, pos, sizeof(U16));
      pos += sizeof(U16);
      
      R_Texture *texture = r_backend_texture_create_impl(sheet_width, sheet_height,
                                                         R_TextureFormat_RGBA8, 4, 0);
      
      if (texture && spr_count) {
        G_Sprite *sprites = ArenaPushArray(arena, G_Sprite, spr_count);
        for (U32 spr_idx = 0; spr_idx < spr_count; spr_idx += 1) {
          G_Sprite *sprite = &sprites[spr_idx];
          
          U16 x = 0;
          MemoryCopy(&x, pos, sizeof(U16));
          pos += sizeof(U16);
          
          U16 y = 0;
          MemoryCopy(&y, pos, sizeof(U16));
          pos += sizeof(U16);
          
          U16 w = 0;
          MemoryCopy(&w, pos, sizeof(U16));
          pos += sizeof(U16);
          
          U16 h = 0;
          MemoryCopy(&h, pos, sizeof(U16));
          pos += sizeof(U16);
          
          U32 duration_ms = 0;
          MemoryCopy(&duration_ms, pos, sizeof(U32));
          pos += sizeof(U32);
          
          sprite->texture = texture;
          sprite->uv_rect.x0 = (F32)x / (F32)sheet_width;
          sprite->uv_rect.y0 = (F32)y / (F32)sheet_height;
          sprite->uv_rect.x1 = (F32)(x + w) / (F32)sheet_width;
          sprite->uv_rect.y1 = (F32)(y + h) / (F32)sheet_height;
        }
        
        r_texture_update(texture, pos, 0, 0, sheet_width, sheet_height, R_TextureFormat_RGBA8);
        
        sheet->sprites = sprites;
        sheet->count = spr_count;
        sheet->width = (S32)sheet_width;
        sheet->height = (S32)sheet_height;
      }
    }
    
    arena_scratch_end(scratch);
  }
}

function G_Asset *
g_assets_alloc_asset(G_AssetContext *ctx, String8 name)
{
  G_Asset *result = 0;
  
  U32 hash = (U32)hash_from_str8(name);
  U32 idx = hash % G_ASSET_SLOTS_MAX;
  
  G_AssetSlot *slot = &ctx->slots[idx];
  G_AssetNode *node = ArenaPushStruct(ctx->arena, G_AssetNode);
  node->v.kind = G_AssetKind_Sprite;
  node->v.name = name;
  node->v.hash = hash;
  
  SLLQueuePushN(ctx->first, ctx->last, node, list_next);
  SLLQueuePushN(slot->first, slot->last, node, hash_next);
  ctx->assets_count += 1;
  
  result = &node->v;
  return result;
}

function G_Sprite *
g_assets_sprite_load(G_AssetContext *ctx, String8 name)
{
  G_Sprite *result = 0;
  
  G_Asset *asset = g_assets_alloc_asset(ctx, name);
  
  String8 path = str8_pushf(ctx->arena, "%.*s/%.*s", ctx->directory.count, ctx->directory.data, name.count, name.data);
  asset->path = path;
  g_assets_sprite_load_impl(&asset->sprite, path);
  
  result = &asset->sprite;
  return result;
}

function G_SpriteSheet *
g_assets_spritesheet_load(G_AssetContext *ctx, String8 name)
{
  G_SpriteSheet *result = 0;
  
  G_Asset *asset = g_assets_alloc_asset(ctx, name);
  
  String8 path = str8_pushf(ctx->arena, "%.*s/%.*s", ctx->directory.count, ctx->directory.data, name.count, name.data);
  asset->path = path;
  g_assets_spritesheet_load_impl(ctx->arena, &asset->spritesheet, path);
  
  result = &asset->spritesheet;
  return result;
}

//
// Asset context
//

function G_AssetContext *
g_assets_alloc(String8 directory)
{
  G_AssetContext *ctx = 0;
  Arena *arena = arena_alloc_default();
  
  ctx = ArenaPushStruct(arena, G_AssetContext);
  ctx->arena = arena;
  
  ctx->directory = directory;
  
  return ctx;
}

function void
g_assets_release(G_AssetContext *ctx)
{
  if (ctx) {
    arena_release(ctx->arena);
  }
}

//
// Asset retrievers
//

function G_Asset *
g_assets_get(G_AssetContext *ctx, String8 name)
{
  G_Asset *result = 0;
  U32 hash = (U32)hash_from_str8(name);
  
  for (G_AssetNode *n = ctx->first; n != 0; n = n->list_next) {
    G_Asset *asset = &n->v;
    if (asset->hash == hash) {
      result = asset;
      break;
    }
  }
  
  return result;
}

function G_Sprite *
g_assets_get_sprite(G_AssetContext *ctx, String8 name)
{
  G_Sprite *result = 0;
  
  G_Asset *asset = g_assets_get(ctx, name);
  if (asset) {
    if (asset->kind == G_AssetKind_Sprite) {
      if (asset->dirty) {
        g_assets_sprite_load_impl(&asset->sprite, asset->path);
        asset->dirty = 0;
      }
      result = &asset->sprite;
    }
  }
  else {
    result = g_assets_sprite_load(ctx, name);
  }
  
  return result;
}

function G_SpriteSheet *
g_assets_get_spritesheet(G_AssetContext *ctx, String8 name)
{
  G_SpriteSheet *result = 0;
  
  G_Asset *asset = g_assets_get(ctx, name);
  if (asset) {
    if (asset->kind == G_AssetKind_SpriteSheet) {
      if (asset->dirty) {
        g_assets_spritesheet_load_impl(ctx->arena, &asset->spritesheet, asset->path);
        asset->dirty = 0;
      }
      result = &asset->spritesheet;
    }
  }
  else {
    result = g_assets_spritesheet_load(ctx, name);
  }
  
  return result;
}
