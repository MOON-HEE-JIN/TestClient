#pragma once

#define __CONNECT_LOGIN_DOCKER__ 1

enum EACTION
{
	EACTION_IDLE,
	EACTION_MOVE,
};

enum ELOGIC_MSG
{
	ELOGIC_TELEPORT,
	ELOGIC_MOVE_STOP,
	ELOGIC_CREATE_OTHER_PLAYER,
	ELOGIC_REMOVE_OTHER_PLAYER,
	ELOGIC_MOVE_START_OTHER_PLAYER,
	ELOGIC_MOVE_STOP_OTHER_PLAYER,
};

enum eKIND_SOCKET
{
	Game,
	Chat,
};

constexpr int MAP_WIDTH = 1024;
constexpr int MAP_HEIGHT = 1024;
constexpr int TILE_SIZE = 64;
constexpr int TILE_COUNT_X = MAP_WIDTH / TILE_SIZE;
constexpr int TILE_COUNT_Y = MAP_HEIGHT / TILE_SIZE;
constexpr int GRID_COUNT_X = 4;
constexpr int GRID_COUNT_Y = 4; // World Z axis
constexpr int GRID_COUNT = GRID_COUNT_X * GRID_COUNT_Y;

constexpr int GridIDFromTile(int tileX, int tileY) noexcept
{
    const int gridX = (tileX * GRID_COUNT_X) / TILE_COUNT_X;
    const int gridY = (tileY * GRID_COUNT_Y) / TILE_COUNT_Y;
    return gridY * GRID_COUNT_X + gridX;
}
constexpr int PLAYER_SIZE = 20;
constexpr int AOI_TILE_RADIUS = 1;

enum ERENDER_MSG
{
	ERENDER_LOCAL_PLAYER_POS,
	ERENDER_SET_LOCAL_PLAYER_ID,
	ERENDER_SET_ENTITY,
	ERENDER_REMOVE_ENTITY,
	ERENDER_CLEAR_ENTITIES,
};
