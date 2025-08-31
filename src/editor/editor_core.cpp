function void
ed_tilemap_column_append_left(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemorySet(map->tokens, G_TileToken_Empty, sizeof(G_TileToken)*tiles_count);
  map->width += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*(width+1)+1;
    S32 src_off = row*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_column_append_right(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  // TODO: Consider filling with floor tiles instead of blank tiles.
  MemorySet(map->tokens, G_TileToken_Empty, sizeof(G_TileToken)*tiles_count);
  map->width += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*(width+1);
    S32 src_off = row*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_column_remove_left(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  S32 new_width = width - 1;
  S32 row_size = sizeof(G_TileToken)*new_width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
  map->width = new_width;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*new_width;
    S32 src_off = row*width+1;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
  }
}

function void
ed_tilemap_column_remove_right(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  
  S32 new_width = width - 1;
  S32 row_size = sizeof(G_TileToken)*new_width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
  map->width = new_width;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 dst_off = row*new_width;
    S32 src_off = row*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
  }
}

function void
ed_tilemap_row_append_top(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  
  MemoryCopyStruct(tmp, map);
  
  MemorySet(map->tokens, G_TileToken_Empty, sizeof(G_TileToken)*tiles_count);
  map->height += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 src_off = row*width;
    S32 dst_off = (row+1)*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, src_row_size); 
  }
}

function void
ed_tilemap_row_append_bottom(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 src_row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemorySet(map->tokens, G_TileToken_Empty, sizeof(G_TileToken)*tiles_count);
  map->height += 1;
  
  for (S32 row = 0; row < height; row += 1) {
    S32 off = row*width;
    MemoryCopy(map->tokens+off, tmp->tokens+off, src_row_size); 
  }
}

function void
ed_tilemap_row_remove_top(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 row_size = sizeof(G_TileToken)*width;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
  map->height -= 1;
  
  for (S32 row = 1; row < height; row += 1) {
    S32 src_off = row*width;
    S32 dst_off = (row-1)*width;
    
    MemoryCopy(map->tokens+dst_off, tmp->tokens+src_off, row_size); 
  }
}

function void
ed_tilemap_row_remove_bottom(Arena *arena, G_TileMap *map)
{
  S32 width = map->width;
  S32 height = map->height;
  S32 tiles_count = width*height;
  S32 row_size = sizeof(G_TileToken)*width;
  S32 new_height = height - 1;
  
  G_TileMap *tmp = ArenaPushStruct(arena, G_TileMap);
  MemoryCopyStruct(tmp, map);
  
  MemoryZero(map->tokens, sizeof(G_TileToken)*tiles_count);
  map->height = new_height;
  
  for (S32 row = 0; row < new_height; row += 1) {
    S32 off = row*width;
    
    MemoryCopy(map->tokens+off, tmp->tokens+off, row_size); 
  }
}
