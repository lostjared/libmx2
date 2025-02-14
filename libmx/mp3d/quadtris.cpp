
#include"quadtris.hpp"
#include <cstdlib>
#include <ctime>
#include <iostream>


namespace puzzle {

    Block::Block(): x{0}, y{0}, color{0}, rotate_x{0} {}


    Block::Block(int x, int y, int color) {
        this->x = x;
        this->y = y;
        this->color =  color;
        this->rotate_x = 0.0;
    }

    Block::Block(const Block &b)  :x{b.x}, y{b.y}, color{b.color}, rotate_x{b.rotate_x}{

    }
    Block::Block(Block&& b) :x{b.x}, y{b.y}, color{b.color}, rotate_x{b.rotate_x} {

    }

	Block &Block::operator=(const Block &b) {
        color = b.color;
        x = b.x;
        y =  b.y;
        rotate_x = b.rotate_x;
        return *this;
    }
	
    Block &Block::operator=(Block&& b) {
        color = b.color;
        x = b.x;
        y =  b.y;
        rotate_x = b.rotate_x;
        return *this;
    }

	bool Block::operator==(const Block &b) const {
        if(b.color == color && b.x == x  && b.y == y)
            return true;

        return false;
    }
	
    bool Block::operator==(const int &color) const {
        if(this->color == color)
            return true;
        return false;
    }

    Piece::Piece(GameGrid *g) : x{0}, y{0}, grid{g} {}
	
    void Piece::reset() {
        x = grid->width()/2;
        y = 0;
        for(int i = 0; i < 3; ++i) {
            blocks[i].x = grid->width()/2;
            blocks[i].y = i;
        }
        do {
            blocks[0].color = 1+(rand()%(grid->numBlocks()-1));
            blocks[1].color = 1+(rand()%(grid->numBlocks()-1));
            blocks[2].color = 1+(rand()%(grid->numBlocks()-1));
        } while(blocks[0].color == blocks[1].color && blocks[0].color == blocks[2].color);
        direction = 0;
    }

    void Piece::setPosition(int x, int y) {
        if(checkLocation(x, this->y)) {
            this->x = x;
        }
    }

    void Piece::shiftColors() {
        Block b[3];
        b[0] = blocks[0];
        b[1] = blocks[1];
        b[2] = blocks[2];
        blocks[0] = b[2];
        blocks[1] = b[0];
        blocks[2] = b[1];
    }

    Block *Piece::at(int index) {
        if(index >= 0 && index <= 2)
            return &blocks[index];
        return nullptr;
    }
    

    void Piece::moveLeft() {

        if(checkLocation(x-1,y)) {
            if(x > 0) {
                x --;
            }
        }
    }

		
    void Piece::moveRight() {
        if(checkLocation(x+1, y)) {
            if(x < grid->width()-1) {
                x ++;
            }
        }
    }

		
    void Piece::moveDown() {
        if(checkLocation(x, y+1)) {
            int stop = (direction == 0) ? 3 : 0;
            if(y < grid->height()-stop) {
                y++;
            } else {
                setBlock();
            }
        } else {
            setBlock();
        }
    }

    void Piece::drop() {
        for(int y = getY(); y < grid->height(); ++y) {
            if(checkLocation(getX(), y)) {
                setBlock();
                return;
            }
        }
    }

    void Piece::setCallback(Callback switch_) {
        switch_grid = switch_;
    }

    void Piece::setBlock() {
        Block *b[3] = {nullptr};

        switch(direction) {
            case 0:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x, y+1);
                b[2] = grid->at(x, y+2);
                break;
            case 1:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x+1, y);
                b[2] = grid->at(x+2, y);
                break;
            case 2:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x-1, y);
                b[2] = grid->at(x-2, y);
                break;
            case 3:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x, y-1);
                b[2] = grid->at(x, y-2);
                break;

        }

        if(b[0] != nullptr &&  b[1] != nullptr && b[2] != nullptr) {
            b[0]->color = blocks[0].color;
            b[1]->color = blocks[1].color;
            b[2]->color = blocks[2].color;
            reset();
            switch_grid();
        }
    }

    bool Piece::checkLocation(int x, int y) {

        Block *b[3] = {nullptr};
        switch(direction) {
            case 0:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x, y+1);
                b[2] = grid->at(x, y+2);
                break;
            case 1:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x+1, y);
                b[2] = grid->at(x+2, y);
                break;
            case 2:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x-1, y);
                b[2] = grid->at(x-2, y);
                break;
            case 3:
                b[0] = grid->at(x, y);
                b[1] = grid->at(x, y-1);
                b[2] = grid->at(x, y-2);
                break;
        }

        if(b[0] != nullptr && b[1] != nullptr && b[2] != nullptr) {
            if(b[0]->color == 0 && b[1]->color == 0 && b[2]->color == 0)
                return true;
        }
        return false;
    }

    void Piece::shiftDirection() {
        int old_dir = direction;
        direction ++;
        if(direction > 3)
            direction = 0;

        if(!checkLocation(x, y)) {
            direction = old_dir;
        }
    }

    GameGrid::GameGrid()  : blocks{nullptr}, grid_w{0}, grid_h{0}, game_piece{this} {
    }

    GameGrid::~GameGrid() {
        releaseGrid();
    }

    void GameGrid::spin() {
        for(int i = 0; i < grid_w; ++i) {
            for(int z  = 0; z < grid_h; ++z) {
                Block *b = at(i, z);
                if(b != nullptr && b->color == -1) {
                    b->rotate_x += 20.0f;
                    if(b->rotate_x >= 360.0f) {
                        b->rotate_x = 0;
                        b->color = 0;
                    }
                }
            }
        }
    }
    
    void GameGrid::releaseGrid() {
        if(blocks != nullptr) {
            for(int i = 0; i < grid_w; ++i) {
                delete [] blocks[i];
            }
            delete [] blocks;
            blocks = nullptr;
        }
    }

    void GameGrid::initGrid(int size_x, int size_y) {
        if(blocks != nullptr)
            releaseGrid();

        blocks = new Block*[size_x];
        for(int i = 0; i < size_x; ++i) {
            blocks[i] = new Block[size_y];
        }
        grid_w = size_x;
        grid_h = size_y;
        game_piece.reset();
    }
    
    Block *GameGrid::at(int x, int y) {
        if(x >= 0  && x < grid_w && y >= 0 && y < grid_h && blocks != nullptr) {
            return &blocks[x][y];
        }
        return nullptr;
    }

    int GameGrid::width() const {
        return grid_w;
    }

    int GameGrid::height() const {
        return grid_h;
    }

    bool GameGrid::canMoveDown() {
        if(game_piece.getDirection() != 3 && game_piece.checkLocation(game_piece.getX(), game_piece.getY()) == false && game_piece.getY() == 0) {
            return false;
        }
        return true;
    }

    
    PuzzleGame::PuzzleGame(int difficulty) : diff{difficulty} {
        newGame();
    }

    void PuzzleGame::newGame() {
        srand(static_cast<int>(time(0)));
        score = 0;
        switch(diff) {
            case 0:
            timeout = 1200;
            break;
            case 1:
            timeout = 900;
            break; 
            case 2:
            timeout = 650;
            break;
        }
        
        clears = 0;
        grid[0].initGrid(12, (720/16/2)+1);
        grid[1].initGrid(12, 28);
        grid[2].initGrid(12, 720/16/2);
        grid[3].initGrid(12, 28);
    }

    void PuzzleGame::setCallback (Callback callback) {
        grid[0].game_piece.setCallback(callback);
        grid[1].game_piece.setCallback(callback);
        grid[2].game_piece.setCallback(callback);
        grid[3].game_piece.setCallback(callback);
    }

    void PuzzleGame::procBlocks() {

        if (timeout > 1100) {
            level = 0;
        } else if (timeout <= 1100 && timeout > 1000) {
            level = 1;
        } else if (timeout <= 1000 && timeout > 900) {
            level = 2;
        } else if (timeout <= 900 && timeout > 800) {
            level = 3;
        } else if (timeout <= 800 && timeout > 700) {
            level = 4;
        } else if (timeout <= 700 && timeout > 600) {
            level = 5;
        }else if (timeout <= 600 && timeout > 500) {
            level = 6;
        }else if (timeout <= 500 && timeout > 400) {
            level = 7;
        }else if (timeout <= 400 && timeout > 300) {
            level = 8;
        }else if (timeout <= 300 && timeout > 200) {
            level = 9;
        }else if (timeout <= 200) {
            level = 10;
        }

        for(int j = 0; j < 4; ++j) {
            for(int i = 0; i < grid[j].width(); ++i) {
                for(int z = 0; z < grid[j].height(); ++z) {
                    Block *blocks[4];
                    blocks[0] = grid[j].at(i, z);
                    blocks[1] = grid[j].at(i, z+1);
                    blocks[2] = grid[j].at(i, z+2);
                    blocks[3] = grid[j].at(i, z+3);

                    if(blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                        if(blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                            if(blocks[3] != nullptr) {
                                if(blocks[0]->color == blocks[3]->color) {
                                    blocks[3]->color = -1;
                                    score += 10;
                                    #ifdef HAS_SOUND
                                        snd::playsound(0);
                                    #endif
                
                                }
                            }
                            blocks[0]->color = -1;
                            blocks[1]->color = -1;
                            blocks[2]->color = -1;
                            score += 1;
                            clears ++;
                            if((clears%4)==0) {
                                timeout -= 25;
                            }

                            #ifdef HAS_SOUND
                            snd::playsound(0);
                            #endif

                            return;
                        }
                    }

                    blocks[0] = grid[j].at(i, z);
                    blocks[1] = grid[j].at(i+1, z);
                    blocks[2] = grid[j].at(i+2, z);
                    blocks[3] = grid[j].at(i+3, z);

                    if(blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                        if(blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                            if(blocks[3] != nullptr) {
                                if(blocks[0]->color == blocks[3]->color) {
                                    blocks[3]->color = -1;
                                    score += 10;
                                    #ifdef HAS_SOUND
                                        snd::playsound(0);
                                    #endif

                                }
                            }
                            blocks[0]->color = -1;
                            blocks[1]->color = -1;
                            blocks[2]->color = -1;
                            score += 1;
                            clears ++;
                            if((clears%4)==0) {
                                timeout -= 25;
                            }

                            #ifdef HAS_SOUND
                            snd::playsound(0);
                            #endif


                            return;
                        }
                    }

                    blocks[0] = grid[j].at(i, z);
                    blocks[1] = grid[j].at(i+1, z+1);
                    blocks[2] = grid[j].at(i+2, z+2);
                    blocks[3] = grid[j].at(i+3, z+3);

                    if (blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                        if (blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                            if (blocks[3] != nullptr) {
                                if (blocks[0]->color == blocks[3]->color) {
                                    blocks[3]->color = -1;
                                    score += 10;
                                    #ifdef HAS_SOUND
                                        snd::playsound(0);
                                    #endif

                                }
                            }
                            blocks[0]->color = -1;
                            blocks[1]->color = -1;
                            blocks[2]->color = -1;
                            score += 1;
                            clears ++;
                            if((clears%4)==0) {
                                timeout -= 25;
                            }
                            return;
                        }
                    }
                
                    blocks[0] = grid[j].at(i, z);
                    blocks[1] = grid[j].at(i-1, z+1);
                    blocks[2] = grid[j].at(i-2, z+2);
                    blocks[3] = grid[j].at(i-3, z+3);

                    if (blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                        if (blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                            if (blocks[3] != nullptr) {
                                if (blocks[0]->color == blocks[3]->color) {
                                    blocks[3]->color = -1;
                                    score += 10;
                                    #ifdef HAS_SOUND
                                        snd::playsound(0);
                                    #endif

                                }
                            }
                            blocks[0]->color = -1;
                            blocks[1]->color = -1;
                            blocks[2]->color = -1;
                            score += 1;
                            clears ++;
                            if((clears%4)==0) {
                                timeout -= 25;
                            }

                            #ifdef HAS_SOUND
                            snd::playsound(0);
                            #endif


                            return;
                        }
                    }
                }
            }
        }

        for(int i = 0; i < grid[0].width(); i++) {
            puzzle::Block *b[4];
            b[0] = grid[0].at(i, grid[0].height()-1);
            b[1] = grid[0].at(i, grid[0].height()-2);
            b[2] = grid[2].at(i, grid[2].height()-1);
            b[3] = grid[2].at(i, grid[2].height()-2);

            if(b[0] && b[1] && b[2]) {
                if(b[0]->color > 0 && b[0]->color == b[1]->color && b[0]->color == b[2]->color) {
                    if(b[0]->color > 0 && b[3] && b[0]->color == b[3]->color) {
                        score += 10;
                        b[3]->color = -1;
                        #ifdef HAS_SOUND
                            snd::playsound(0);
                        #endif
                    }

                    score++;
                    clears++;
                    if((clears%4)==0) {
                        timeout -= 25;
                    }
                    b[0]->color = -1;
                    b[1]->color = -1;
                    b[2]->color = -1;

                        #ifdef HAS_SOUND
                        snd::playsound(0);
                        #endif


                    return;
                }
            }
            if(b[2] && b[3] && b[0]) {
                if(b[2]->color > 0 && b[2]->color == b[0]->color && b[2]->color == b[3]->color) {
                    if(b[1] && b[2]->color == b[1]->color) {
                        score += 10;
                        b[1]->color = -1;
                        #ifdef HAS_SOUND
                            snd::playsound(0);
                        #endif

                    }

                    score ++;
                    clears++;
                    if((clears%4)==0) {
                        timeout -= 25;
                    }
                    b[2]->color = -1;
                    b[3]->color = -1;
                    b[0]->color = -1;

                    #ifdef HAS_SOUND
                    snd::playsound(0);
                    #endif
                }
            }
        }
      
        for(int i = 0; i < grid[0].width() - 3; ++i) {
            puzzle::Block *b[4];
            int y0 = grid[0].height() - 1; 
            int y1 = grid[2].height() - 1; 

            b[0] = grid[0].at(i, y0);
            b[1] = grid[2].at(i + 1, y1);
            b[2] = grid[2].at(i + 2, y1 - 1);
            b[3] = grid[2].at(i + 3, y1 - 2);

            if(b[0] && b[1] && b[2]) {
                if(b[0]->color > 0 && b[0]->color == b[1]->color && b[0]->color == b[2]->color) {
                    if(b[3] && b[0]->color == b[3]->color) {
                        score += 10;
                        b[3]->color = -1;
                        #ifdef HAS_SOUND
                            snd::playsound(0);
                        #endif
                    }

                    score++;
                    clears++;
                    if((clears % 4) == 0) {
                        timeout -= 25;
                    }
                    b[0]->color = -1;
                    b[1]->color = -1;
                    b[2]->color = -1;
                    #ifdef HAS_SOUND
                    snd::playsound(0);
                    #endif
                    return;
                }
            }
        }

        for(int i = 3; i < grid[0].width(); ++i) {
            puzzle::Block *b[4];
            int y0 = grid[0].height() - 1; 
            int y1 = grid[2].height() - 1; 

            b[0] = grid[0].at(i, y0);
            b[1] = grid[2].at(i - 1, y1);
            b[2] = grid[2].at(i - 2, y1 - 1);
            b[3] = grid[2].at(i - 3, y1 - 2);

            if(b[0] && b[1] && b[2]) {
                if(b[0]->color > 0 && b[0]->color == b[1]->color && b[0]->color == b[2]->color) {
                    if(b[3] && b[0]->color == b[3]->color) {
                        score += 10;
                        b[3]->color = -1;
                        #ifdef HAS_SOUND
                            snd::playsound(0);
                        #endif
                    }
                    score++;
                    clears++;
                    if((clears % 4) == 0) {
                        timeout -= 25;
                    }
                    b[0]->color = -1;
                    b[1]->color = -1;
                    b[2]->color = -1;
                    #ifdef HAS_SOUND
                    snd::playsound(0);
                    #endif
                    return;
                }
            }
        }
        moveDown_Blocks();
    }
    	
    void PuzzleGame::moveDown_Blocks() {
        for(int j = 0; j < 4; ++j) {
            for(int i = 0; i < grid[j].width(); ++i) {
                for(int z = 0; z < grid[j].height(); ++z) {
                    Block *b1 = grid[j].at(i, z);
                    Block *b2 = grid[j].at(i, z+1);
                    if(b1 != nullptr && b2 != nullptr) {
                        if(b2->color == 0 && b1->color > 0) {
                            std::swap(b1->color, b2->color);
                            return;
                        }
                    }

                }
            }
        }
    }

    MasterPiece::MasterPiece(int difficulty) : diff{difficulty} {
        newGame();
    }
    void MasterPiece::setCallback (Callback callback) {
        grid.game_piece.setCallback(callback);
    }
    void MasterPiece::newGame() {
        srand(static_cast<int>(time(0)));
        score = 0;
        switch(diff) {
            case 0:
            timeout = 1200;
            break;
            case 1:
            timeout = 900;
            break; 
            case 2:
            timeout = 650;
            break;
        }
        
        clears = 0;
        grid.initGrid(15, 18);
    }
    void MasterPiece::procBlocks() {
        if (timeout > 1100) {
            level = 0;
        } else if (timeout <= 1100 && timeout > 1000) {
            level = 1;
        } else if (timeout <= 1000 && timeout > 900) {
            level = 2;
        } else if (timeout <= 900 && timeout > 800) {
            level = 3;
        } else if (timeout <= 800 && timeout > 700) {
            level = 4;
        } else if (timeout <= 700 && timeout > 600) {
            level = 5;
        }else if (timeout <= 600 && timeout > 500) {
            level = 6;
        }else if (timeout <= 500 && timeout > 400) {
            level = 7;
        }else if (timeout <= 400 && timeout > 300) {
            level = 8;
        }else if (timeout <= 300 && timeout > 200) {
            level = 9;
        }else if (timeout <= 200) {
            level = 10;
        }

        for(int i = 0; i < grid.width(); ++i) {
            for(int z = 0; z < grid.height(); ++z) {
                Block *blocks[4];
                blocks[0] = grid.at(i, z);
                blocks[1] = grid.at(i, z+1);
                blocks[2] = grid.at(i, z+2);
                blocks[3] = grid.at(i, z+3);

                if(blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                    if(blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                        if(blocks[3] != nullptr) {
                            if(blocks[0]->color == blocks[3]->color) {
                                blocks[3]->color = -1;
                                score += 10;
                                #ifdef HAS_SOUND
                                    snd::playsound(0);
                                #endif
            
                            }
                        }
                        blocks[0]->color = -1;
                        blocks[1]->color = -1;
                        blocks[2]->color = -1;
                        score += 1;
                        clears ++;
                        if((clears%4)==0) {
                            timeout -= 25;
                        }

                        #ifdef HAS_SOUND
                        snd::playsound(0);
                        #endif

                        return;
                    }
                }

                blocks[0] = grid.at(i, z);
                blocks[1] = grid.at(i+1, z);
                blocks[2] = grid.at(i+2, z);
                blocks[3] = grid.at(i+3, z);

                if(blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                    if(blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                        if(blocks[3] != nullptr) {
                            if(blocks[0]->color == blocks[3]->color) {
                                blocks[3]->color = -1;
                                score += 10;
                                #ifdef HAS_SOUND
                                    snd::playsound(0);
                                #endif

                            }
                        }
                        blocks[0]->color = -1;
                        blocks[1]->color = -1;
                        blocks[2]->color = -1;
                        score += 1;
                        clears ++;
                        if((clears%4)==0) {
                            timeout -= 25;
                        }

                        #ifdef HAS_SOUND
                        snd::playsound(0);
                        #endif


                        return;
                    }
                }

                blocks[0] = grid.at(i, z);
                blocks[1] = grid.at(i+1, z+1);
                blocks[2] = grid.at(i+2, z+2);
                blocks[3] = grid.at(i+3, z+3);

                if (blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                    if (blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                        if (blocks[3] != nullptr) {
                            if (blocks[0]->color == blocks[3]->color) {
                                blocks[3]->color = -1;
                                score += 10;
                                #ifdef HAS_SOUND
                                    snd::playsound(0);
                                #endif

                            }
                        }
                        blocks[0]->color = -1;
                        blocks[1]->color = -1;
                        blocks[2]->color = -1;
                        score += 1;
                        clears ++;
                        if((clears%4)==0) {
                            timeout -= 25;
                        }
                        return;
                    }
                }
            
                blocks[0] = grid.at(i, z);
                blocks[1] = grid.at(i-1, z+1);
                blocks[2] = grid.at(i-2, z+2);
                blocks[3] = grid.at(i-3, z+3);

                if (blocks[0] != nullptr && blocks[1] != nullptr && blocks[2] != nullptr) {
                    if (blocks[0]->color > 0 && blocks[0]->color == blocks[1]->color && blocks[0]->color == blocks[2]->color) {
                        if (blocks[3] != nullptr) {
                            if (blocks[0]->color == blocks[3]->color) {
                                blocks[3]->color = -1;
                                score += 10;
                                #ifdef HAS_SOUND
                                    snd::playsound(0);
                                #endif

                            }
                        }
                        blocks[0]->color = -1;
                        blocks[1]->color = -1;
                        blocks[2]->color = -1;
                        score += 1;
                        clears ++;
                        if((clears%4)==0) {
                            timeout -= 25;
                        }

                        #ifdef HAS_SOUND
                        snd::playsound(0);
                        #endif

                        return;
                    }
                }
            }
        }
        moveDown_Blocks();
    }

    void MasterPiece::moveDown_Blocks() {
        for(int i = 0; i < grid.width(); ++i) {
            for(int z = 0; z < grid.height(); ++z) {
                Block *b1 = grid.at(i, z);
                Block *b2 = grid.at(i, z+1);
                if(b1 != nullptr && b2 != nullptr) {
                    if(b2->color == 0 && b1->color > 0) {
                        std::swap(b1->color, b2->color);
                        return;
                    }
                }
            }
        }
        drop = false;
    }
}

