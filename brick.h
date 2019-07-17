#ifndef BRICK_H
#define BRICK_H
#include <SDL.h>
#include <vector>
#include <ctime>
#include <cstdlib>
using namespace std;

class GameBoard;

constexpr int BLANK_GRID = 0;

constexpr int NO_COLLISION = 0;
constexpr int COLLISION_OUT_OF_BOARD = 1;
constexpr int COLLISION_WITH_BRICK = 2;
constexpr int COLLISION_GAMEOVER = 3;

const vector<vector<Uint32>> COLORS = {
	{0x27, 0x1a, 0xab, 0xff},
	{0x92, 0x14, 0x0c, 0xff},
	{0xff, 0x55, 0x00, 0xff},
	{0x48, 0xa9, 0x0a, 0xff},
};

extern int xy2i(int,int,int);
extern int i2x(int,int);
extern int i2y(int,int);
int inrange(int x, int y, int ncols, int nrows){
	return x >= 0 && x < ncols && y >= 0 && y < nrows;
}

struct GridInfo {
	int _color;
	SDL_Point _pos;

	GridInfo(int color, SDL_Point pos) : _color(color), _pos(pos) {}
	GridInfo(int color, int x, int y) : _color(color), _pos{x, y} {}
};

class GameBoard {
public:
	GameBoard(int nrows, int ncols) : _nrows(nrows), _ncols(ncols),
		_nbGrids(nrows * ncols), _board(_nbGrids, BLANK_GRID) {
	}

	void setBoard(int i, int state) {
		_board[i] = state;
	}

	int getBoard(int i){
		return _board[i];
	}

	int getRows()const { return _nrows; }

	int getCols()const { return _ncols; }

	int getNbGrids()const { return _nbGrids; }

	void render(vector<GridInfo> &board) {
		for (int i = 0; i < _nbGrids; i++) {
			if (_board[i] != BLANK_GRID)
				board.emplace_back(_board[i], i % _ncols, i / _ncols);
		}
	}

	int handleFilledLines() {
		int emptyLine = _nrows - 1;
		int nbFilledLines = 0;
		for (int i = _nrows - 1; i >= 0; i--) {
			bool filled = true;
			for (int j = 0; j < _ncols; j++) {
				if (_board[xy2i(j, i, _ncols)] == BLANK_GRID) {
					filled = false;
					break;
				}
			}
			if (!filled) {
				for (int j = 0; j < _ncols; j++) {
					_board[xy2i(j, emptyLine, _ncols)] = _board[xy2i(j, i, _ncols)];
				}
				emptyLine--;
			}
			else {
				nbFilledLines++;
			}
		}
		emptyLine = emptyLine >= 0 ? emptyLine : 0; // in case none of the lines are filled
		fill(_board.begin(), _board.begin() + emptyLine * _ncols, BLANK_GRID);
		return nbFilledLines;
	}

private:
	int _nrows = 0;
	int _ncols = 0;
	int _nbGrids = 0;
	vector<int> _board;
};

/* You can come up with any shape that could fit inside the 4 * 4 rectangle.
 *
 * There are 5 types of bricks in use:
 *  1100   1111    1100    1100    0100
 *  1100   0000    0110    1000    1110
 *  0000   0000    0000    1000    0000
 *  0000   0000    0000    0000    0000
 */
const vector<vector<int>> BRICKS = {
	{1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{1, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
	{0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};

const vector<vector<int>> PERMUTATIONS = {
	{12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3}, // rotate right
	{3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12}, // rotate left
	{3, 2, 1, 0, 7, 6, 5, 4, 11, 10, 9, 8, 15, 14, 13, 12}, // flip left right
};

enum Motion{
	NO_ACTION, GO_LEFT, GO_RIGHT, GO_DOWN, ROTATE_LEFT, ROTATE_RIGHT, FLIP
};

class Brick{
public:
	Brick() = default;

	Brick(int brickType, int color, int x=0, int y=0): _contents(BRICKS[brickType]), _color(color), _pos{x, y} {
	}

	// flip left right
	void flip(){
	    permute(_contents, PERMUTATIONS[2]);
	}

	void rotateLeft(){
		permute(_contents, PERMUTATIONS[1]);
	}

	void rotateRight(){
		permute(_contents, PERMUTATIONS[0]);
	}

	// keep moving down until you hit something
	void moveDown(GameBoard &gb){
		int nrows = gb.getRows();
		for (int i = 0; i < nrows; i++) {
			move(0, 1);
			if (collideWith(gb) != NO_COLLISION) {
				move(0, -1);
				break;
			}
		}
	}

	void move(int dx, int dy) {
		_pos.x += dx;
		_pos.y += dy;
	}

	void move(Motion motion, GameBoard &gb){
		switch(motion){
			case NO_ACTION:
				break;
			case GO_LEFT:
				moveIfNoCollision(gb, -1, 0);
				break;
			case GO_RIGHT:
				moveIfNoCollision(gb, 1, 0);
				break;
			case GO_DOWN:
				moveDown(gb);
				break;
			case ROTATE_RIGHT:
				rotateRightIfNoCollision(gb);
				break;
			case FLIP:
				flipIfNoCollision(gb);
				break;
			default:
				break;
		}
	}

	void moveIfNoCollision(GameBoard &gb, int dx, int dy){
		move(dx, dy);
		if(collideWith(gb))
			move(-dx, -dy);
	}

	void flipIfNoCollision(GameBoard &gb){
		flip();
		if(collideWith(gb))
			flip();
	}

	void rotateLeftIfNoCollision(GameBoard &gb){
		rotateLeft();
		if(collideWith(gb))
			rotateRight();
	}

	void rotateRightIfNoCollision(GameBoard &gb){
		rotateRight();
		if(collideWith(gb))
			rotateLeft();
	}

	int collideWith(GameBoard &gb) {
		vector<GridInfo> grids = getGrids();
		int ncols = gb.getCols(), nrows = gb.getRows();
		int nbGrids = gb.getNbGrids();
		int x, y, p;
		for (GridInfo &info : grids) {
			x = info._pos.x;
			y = info._pos.y;
			p = xy2i(x, y, ncols);
			if (x < 0 || x >= ncols || y >= nrows)
				return COLLISION_OUT_OF_BOARD;
			if (p >= 0 && p < nbGrids && gb.getBoard(p) != BLANK_GRID)
				return COLLISION_WITH_BRICK;
		}
		return NO_COLLISION;
	}

	/*
	 * This function append the grids occupied by the current brick to
	 * the vector `toRender`.
	 *
	 */
	void render(vector<GridInfo> &toRender, GameBoard &gb) {
		auto grids = getGrids();
		int ncols = gb.getCols(), nrows = gb.getRows();
		for (GridInfo &info : grids) {
			if (inrange(info._pos.x, info._pos.y, ncols, nrows))
				toRender.emplace_back(_color, info._pos.x, info._pos.y);
		}
	}

	/*
	 * This function is used to update the game board with the grids
	 * occupied by the current brick. This happens when the brick
	 * reaches its lowest level.
	 *
	 */
	void updateBoard(GameBoard &gb) {
		auto grids = getGrids();
		int ncols = gb.getCols(), nrows = gb.getRows();
		for (GridInfo &info : grids) {
			int p = xy2i(info._pos.x, info._pos.y, ncols);
			if (inrange(info._pos.x, info._pos.y, ncols, nrows))
				gb.setBoard(p, _color);
		}
	}

	/*
	 * This function returns a vector containing all the grids the current
	 * brick occupies, including those not inside the game board.
	 */
	vector<GridInfo> getGrids() {
		vector<GridInfo> grids;
		int pRef = getReferenceGridPosition();
		int ri = pRef / 4, rj = pRef % 4;
		int pi, pj;
		for (int i = 0; i < 16; i++) {
			if (_contents[i]) {
				pi = i / 4;
				pj = i % 4;
				grids.emplace_back(_color, _pos.x + pj - rj, _pos.y + pi - ri);
			}
		}
		return grids;
	}

	int detectCollision(GameBoard &gb) {
		move(0, 1);
		int state = collideWith(gb);
		move(0, -1);
		return state;
	}

private:

	vector<int> _contents;
	int _color = 1;
	SDL_Point _pos = { 0, 0 };
	
	void swap(int p1, int p2){
		int tmp = _contents[p1];
		_contents[p1] = _contents[p2];
		_contents[p2] = tmp;
	}

	/*   Currently the permutation is done by utilizing an extra vector. There is an
	 * implementation that could do an inplace permutation. Search on StackOverFlow
	 * for a solution.
	 *
	 * Need extra O(n) space for permutation
	 */
	void permute(vector<int> &a, const vector<int> &p){
		vector<int> newp(p.size());
		int n = newp.size();
		for(int i = 0; i < n; i++)
			newp[i] = a[p[i]];
		a.assign(newp.cbegin(), newp.cend());
	}

	bool isRowEmpty(int row){
		int start = row * 4, end = start + 4;
		for(int i = start; i < end; i++){
		    if(_contents[i])
		    	return false;
		}
		return true;
	}

	bool isColEmpty(int col){
		for(int i = col; i < 16; i += 4){
		    if(_contents[i])
		    	return false;
		}
		return true;
	}

	// left bottom position
	int getReferenceGridPosition(){
		int pi = 4, pj = -1;
		for(int i = 3; i >= 0; i--){
		    if(!isRowEmpty(i)) {
				pi = i;
				break;
			}
		}
		for(int i = 0; i < 4; i++){
		    if(!isColEmpty(i)) {
				pj = i;
				break;
			}
		}
		return pi * 4 + pj;
	}
};

class BrickFactory {
public:
	BrickFactory(bool random=true){
		if(random)
			srand(static_cast<unsigned int>(time(0)));
	}
	Brick nextBrick() {
		return Brick(rand() % BRICKS.size(), rand() % COLORS.size() + 1, 5);
	}
};

#endif // !BRICK_H
