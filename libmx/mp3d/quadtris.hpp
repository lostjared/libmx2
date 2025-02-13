#ifndef PUZZLE_BLOCK_H
#define PUZZLE_BLOCK_H
#include <array>
#include <vector>
#include<functional>

namespace puzzle {


	class Block {
	public:
		int x,y,color;
		Block();
		Block(int x, int y, int color);
		Block(const Block &b);
		Block(Block &&b);
		Block &operator=(const Block &b);
		Block &operator=(Block &&b);
		bool operator==(const Block &b) const;
		bool operator==(const int &c) const;
	};

	class GameGrid;

	using Callback = std::function<void ()>;

	class Piece {
	public:
		Piece(GameGrid *grid);
		void setCallback(Callback switch_);
		void reset();
		void shiftColors();
		int getX() const { return x; }
		int getY() const { return y; }
		int getDirection() const { return direction; }
		Block *at(int index);
		void moveLeft();
		void moveRight();
		void moveDown();
		bool checkLocation(int x, int y);
		void shiftDirection();
		void setBlock();
		void drop();
		void setPosition(int x, int y);
	private:
		Block blocks[3];
		int x = 0,y = 0;
		int direction = 0;
		GameGrid *grid;
		Callback switch_grid = nullptr;
	};

	class GameGrid  {
	public:
		GameGrid();
		~GameGrid();
		void initGrid(int size_x, int size_y);
		void releaseGrid();
		Block *at(int x, int y);
		int width() const;
		int height() const;
		int numBlocks() const { return block_number; }
		bool canMoveDown();
	protected:
		Block **blocks = nullptr;
		int grid_w = 0, grid_h = 0;
		
		
	public:
		Piece game_piece;
	private:
		static const int block_number = 5;
	};

	class PuzzleGame  {
	public:
		PuzzleGame(int difficulty);
		void setCallback (Callback callback);
		void newGame();
		GameGrid grid[4];
		int score = 0;
		unsigned int timeout = 1200;
		int clears = 0;
		int level =  0;
		void procBlocks();
		void moveDown_Blocks();
	private:
		int diff = 0;
	};

	class MasterPiece {
	public:
		MasterPiece(int difficulty);
		void setCallback (Callback callback);
		void newGame();
		GameGrid grid;
		int score = 0;
		unsigned int timeout = 1200;
		int clears = 0;
		int level =  0;
		void procBlocks();
		void moveDown_Blocks();
	private:
		int diff = 0;
	};

}

#endif
