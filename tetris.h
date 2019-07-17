#ifndef TERIS_H
#define TERIS_H

#include <string>
#include <vector>
#include <SDL.h>
#include <memory>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include "brick.h"
using namespace std;

constexpr int GRID_SIZE = 30;
constexpr int MARGIN_SIZE = 5;
constexpr int PREVIEW_SIZE = MARGIN_SIZE;
constexpr int STATUS_LINE_SZIE = 200;
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

		_bgMusic = shared_ptr<Mix_Music>(Mix_LoadMUS("1757.wav"), Mix_FreeMusic);
		_font = shared_ptr<TTF_Font>(TTF_OpenFont("micross.ttf", 28), TTF_CloseFont);
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
	shared_ptr<TTF_Font> _font;
	shared_ptr<SDL_Texture> _textTexture;

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

	double _dx = 0;
	double _v = 0.05;
	Motion _motion = NO_ACTION;
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

	/*
	 * This function only records the action but does not perform it.
	 *
	 * Refer to function @moveBrick that actually perform the action.
	 */
	void handleEvent(){
		SDL_Event e;
		_motion = NO_ACTION;
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)
				_running = false;
			else if (e.type == SDL_KEYDOWN) {
				switch (e.key.keysym.sym) {
				case SDLK_a:
				    _motion = GO_LEFT;
					break;
				case SDLK_d:
				    _motion = GO_RIGHT;
					break;
				case SDLK_s:
					_motion = GO_DOWN;
					break;
				case SDLK_j:
				    _motion = FLIP;
					break;
				case SDLK_k:
					_motion = ROTATE_RIGHT;
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

	/*
	 * Detect whether the brick has reached its lowest level.
	 */
	void detectCollision(){
		int state = _currentBrick.detectCollision(_board);
		if (state != NO_COLLISION) {
			_currentBrick.updateBoard(_board);
			_scores += _board.handleFilledLines();
			_currentBrick = _nextBrick;
			_nextBrick = _brickFactory.nextBrick();

			// if the new brick collide with the game board, GameOver!
			if(_currentBrick.detectCollision(_board) != NO_COLLISION)
				_running = false;
		}
	}

	void renderScreen(){
		drawOutline();
		renderBricks();
		renderScore();
	}

	void renderScore(){
		string scoreText = "Score: ";
		scoreText += to_string(_scores);
		SDL_Color color = {0, 0, 0, 0};
		auto surface = shared_ptr<SDL_Surface>(TTF_RenderText_Solid(_font.get(), scoreText.c_str(), color), SDL_FreeSurface);
		_textTexture = shared_ptr<SDL_Texture>(SDL_CreateTextureFromSurface(gRenderer, surface.get()), SDL_DestroyTexture);

		SDL_Rect rect = {PREVIEW_SIZE + GRID_SIZE * _ncols + 10, 150, 150, 25};
		SDL_RenderCopy(gRenderer, _textTexture.get(), nullptr, &rect);
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

		auto nextGrids = _nextBrick.getGrids();
		for(auto info: nextGrids){
			int x = (info._pos.x + _ncols / 2) * GRID_SIZE + PREVIEW_SIZE + MARGIN_SIZE;
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
	    _currentBrick.move(_motion, _board);

		_dx += _v;
		if(_dx >= 1){
			_dx -= 1;
			_currentBrick.moveIfNoCollision(_board, 0, 1);
		}
	}
};

#endif
