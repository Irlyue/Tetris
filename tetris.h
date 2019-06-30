#ifndef TERIS_H
#define TERIS_H

#include <string>
#include <vector>
#include <SDL.h>
#include <memory>
#include <SDL_mixer.h>
#include "brick.h"
using namespace std;

constexpr int GRID_SIZE = 30;
constexpr int PREVIEW_SIZE = 150;
constexpr int MARGIN_SIZE = 5;
constexpr int STATUS_LINE_SZIE = 150;
constexpr int NEXT_GRID_DX = 50;
constexpr int NEXT_GRID_DY = 30;

SDL_Renderer *gRenderer = nullptr;
SDL_Window *gWindow = nullptr;

int xy2i(int x, int y, int ncols) {
	return y * ncols + x;
}

int i2x(int i, int ncols) {
	return i % ncols;
}

int i2y(int i, int ncols) {
	return i / ncols;
}

void freeMixerMusic(Mix_Music *music){
	Mix_FreeMusic(music);
}

class GameBoard;

class Tetris{
public:
	Tetris(int nrows, int ncols): _nrows(nrows), _ncols(ncols), _board(nrows, ncols){
		_winHeight = _nrows * GRID_SIZE + MARGIN_SIZE * 2;
		_winWidth = _ncols * GRID_SIZE + MARGIN_SIZE * 2 + PREVIEW_SIZE + STATUS_LINE_SZIE;
		_boardHeight = _board.getRows() * GRID_SIZE;
		_boardWidth = _board.getCols() * GRID_SIZE;
		pre_run();
	}

	~Tetris(){
		post_run();
	}


	void pre_run(){
		gWindow = SDL_CreateWindow("Teris", SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED, _winWidth, _winHeight, SDL_WINDOW_SHOWN);
		gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);

		_bgMusic = shared_ptr<Mix_Music>(Mix_LoadMUS("1757.wav"), freeMixerMusic);
	}

	void run(){
		_running = true;
		_startTime = SDL_GetTicks();
		Uint32 tic, elapsed;
		_currentBrick = _brickFactory.nextBrick();
		_nextBrick = _brickFactory.nextBrick();
		_nbFrames = 0;
		startUpMusic();
		while (_running) {
			_nbFrames++;
			tic = SDL_GetTicks();
			handleEvent();
			moveBrick();
			clearScreen();
			renderScreen();
			detectCollision();
			SDL_RenderPresent(gRenderer);
			elapsed = SDL_GetTicks() - tic;
			if (elapsed < 30)
				SDL_Delay(30 - elapsed);
		}
		stopMusic();
	}
	void post_run(){
		SDL_DestroyRenderer(gRenderer);
		SDL_DestroyWindow(gWindow);
	}
private:

	shared_ptr<Mix_Music> _bgMusic;

	bool _running = false;
	Uint32 _startTime = 0;
	int _scores = 0;

	int _nbFrames = 0;
	int _nrows = 0;
	int _ncols = 0;
	int _winHeight = 0;
	int _winWidth = 0;
	int _boardHeight = 0;
	int _boardWidth = 0;
	GameBoard _board;

	BrickFactory _brickFactory;

	Brick _currentBrick;
	Brick _nextBrick;

	void startUpMusic(){
		Mix_PlayMusic(_bgMusic.get(), -1);
	};

	void stopMusic(){
		Mix_HaltMusic();
	}

	void handleEvent(){
		SDL_Event e;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
				_running = false;
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
				case SDLK_a:
					_currentBrick.moveIfNoCollision(_board, -1, 0);
					break;
				case SDLK_d:
					_currentBrick.moveIfNoCollision(_board, 1, 0);
					break;
				case SDLK_s:
					_currentBrick.moveDown(_board);
					break;
				case SDLK_j:
					_currentBrick.flipIfNoCollision(_board);
					break;
				case SDLK_k:
					_currentBrick.rotateRightIfNoCollision(_board);
					break;
				default:
					break;
				}
			}
		}
	}

	void clearScreen(){
		SDL_SetRenderDrawColor(gRenderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(gRenderer);
	}

	void detectCollision(){
		int state = _currentBrick.detectCollision(_board);
		if (state != NO_COLLISION) {
			_currentBrick.updateBoard(_board);
			_scores += _board.handleFilledLines();
			_currentBrick = _nextBrick;
			_nextBrick = _brickFactory.nextBrick();
		}
	}

	void renderScreen(){
		drawOutline();
		renderBricks();
	}

	void drawOutline(){
		int x1 = PREVIEW_SIZE + MARGIN_SIZE;
		int y1 = MARGIN_SIZE;
		SDL_Rect rect = {x1, y1, _boardWidth, _boardHeight};
		SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xff);
		SDL_RenderDrawRect(gRenderer, &rect);
	}

	void renderBricks(){
		vector<GridInfo> grids;
		_currentBrick.render(grids, _board);
		_board.render(grids);

		for(auto &info: grids){
			info._pos.x = info._pos.x * GRID_SIZE + PREVIEW_SIZE + MARGIN_SIZE + 1;
			info._pos.y = info._pos.y * GRID_SIZE + MARGIN_SIZE + 1;
		}

		auto nextGrids = _nextBrick.getGrids(_board);
		for(auto info: nextGrids){
			int x = (info._pos.x + _ncols) * GRID_SIZE + PREVIEW_SIZE + MARGIN_SIZE;
			int y = (info._pos.y + 2) * GRID_SIZE + MARGIN_SIZE + 1;
			grids.emplace_back(info._color, x + NEXT_GRID_DX, y + NEXT_GRID_DY);
		}

		SDL_Rect rect = { 0, 0, GRID_SIZE - 2, GRID_SIZE - 2 };
		for (auto &info : grids) {
			auto &color = COLORS[info._color - 1];
			SDL_SetRenderDrawColor(gRenderer, color[0], color[1], color[2], color[3]);
			rect.x = info._pos.x;
			rect.y = info._pos.y;
			SDL_RenderFillRect(gRenderer, &rect);
		}
	}

	void moveBrick(){
		if (_nbFrames % 20 == 0) {
			_currentBrick.moveIfNoCollision(_board, 0, 1);
		}
	}
};

#endif
