//   g++ SemiFinal1.cpp Sound.cpp -o a.out -lSDL2 -lSDL2_mixer -lSDL2_image -lSDL2_ttf -ldl

#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "Sound.hpp"

int SCREEN_HEIGHT = 720;
int SCREEN_WIDTH = 720;

enum GameState
{
    STARTINGSCREEN,
    PLAYING,
    GAMEOVER
};

enum class Troops
{
    Bishop,
    Knight,
    Rook,
    King,
    Queen,
    Pawn,
    None
};

Sound move_Sound;
Sound attack_Sound;
Sound checkmate_Sound;
Sound kingcheck_Sound;
Sound gamestart_Sound;
Sound Promotion_Sound;

enum class Color
{
    Black,
    White,
    None
};

class Piece
{
public:
    Color color;
    Troops TroopType;
    bool hasMoved;
    Piece(Troops armyType = Troops::None, Color colortype = Color::None) : color(colortype), TroopType(armyType), hasMoved{false} {}
};

class ChessBoard
{
private:
    SDL_Renderer *renderer;
    std::vector<std::vector<Piece>> board;
    std::vector<SDL_Texture *> pieceTextures;       
    std::pair<int, int> selectedPiece = {-1, -1};   // selectedPiece 's x and y dimension to get the actuqal block on the board and check the data on it
    std::vector<std::pair<int, int>> highlightMove; // highlighting block's coorindated vector

public:
    ChessBoard(SDL_Renderer *render) : renderer(render), board(8, std::vector<Piece>(8, Piece()))
    {
        SetupBoard();
        LoadTextures(renderer);
    }

    ~ChessBoard()
    {
        for (auto texture : pieceTextures)
        {
            SDL_DestroyTexture(texture);
        }
    }

    void LoadTextures(SDL_Renderer *renderer)
    {
        pieceTextures.resize(12); 

        // Load Black pieces
        pieceTextures[0] = IMG_LoadTexture(renderer, "textures/Black_Pawn.png");

        pieceTextures[1] = IMG_LoadTexture(renderer, "textures/Black_Rook.png");

        pieceTextures[2] = IMG_LoadTexture(renderer, "textures/Black_Knight.png");
        pieceTextures[3] = IMG_LoadTexture(renderer, "textures/Black_Bishop.png");
        pieceTextures[4] = IMG_LoadTexture(renderer, "textures/Black_Queen.png");
        pieceTextures[5] = IMG_LoadTexture(renderer, "textures/Black_King.png");

        // Load White pieces
        pieceTextures[6] = IMG_LoadTexture(renderer, "textures/White_Pawn.png");
        pieceTextures[7] = IMG_LoadTexture(renderer, "textures/White_Rook.png");
        pieceTextures[8] = IMG_LoadTexture(renderer, "textures/White_Knight.png");
        pieceTextures[9] = IMG_LoadTexture(renderer, "textures/White_Bishop.png");
        pieceTextures[10] = IMG_LoadTexture(renderer, "textures/White_Queen.png");
        pieceTextures[11] = IMG_LoadTexture(renderer, "textures/White_King.png");
    }

    void selectPiece(int x, int y)
    {
        selectedPiece = {x, y};
        Piece &piece = get_PieceAt(x, y); // check this
        highlightMove = getLegalMoves(piece, x, y);
    }

    bool isPieceSelected() const
    {
        return selectedPiece.first != -1;
    }

    void deselectPiece()
    {
        selectedPiece = {-1, -1};
        highlightMove.clear();
    }

    void printBoard()
    {
        for (const auto &rows : board)
        {
            for (const auto &piece : rows)
            {
                const char pieceSymbol = get_PieceAtdata(piece);
                std::cout << pieceSymbol << " ";
            }
            std::cout << std::endl;
        }
    }

    bool simulateMove(int srcX, int srcY, int destX, int destY, Color currentPlayerColor)
    {
        Piece pieceToMove = board[srcY][srcX];
        Piece backupPiece = board[destY][destX];

        board[destY][destX] = pieceToMove;
        board[srcY][srcX] = Piece(Troops::None, Color::None);

        std::pair<int, int> KingPosition = findKingPosition(currentPlayerColor);
        bool inCheck = IsKingCheck(KingPosition.first, KingPosition.second, currentPlayerColor);

        board[srcY][srcX] = pieceToMove;
        board[destY][destX] = backupPiece;

        return !inCheck;
    }

    bool isCheckMate(Color currentPlayerColor)
    {
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                Piece &piece = board[y][x];


                if (piece.color == currentPlayerColor)
                {
                    std::vector<std::pair<int, int>> legalMoves = getLegalMoves(piece, x, y);

                    for (const auto &move : legalMoves)
                    {
                        if (simulateMove(x, y, move.first, move.second, currentPlayerColor))
                        {
                            return false; // A move is possible to save the king
                        }
                    }
                }
            }
        }
        return true;
    }

    bool isStaleMate(Color currentPlayerColor)
    {
        for (int y = 0; y < 8; y++)
        {
            for (int x = 0; x < 8; x++)
            {
                Piece &piece = board[y][x];

                if (piece.color == currentPlayerColor)
                {
                    std::vector<std::pair<int, int>> legalMoves = getLegalMoves(piece, x, y);

                    for (const auto &move : legalMoves)
                    {
                        if (simulateMove(x, y, move.first, move.second, currentPlayerColor))
                        {
                            return false; // A move is possible
                        }
                    }
                }
            }
        }

        return true;
    }

    bool isInsideBoard(int x, int y)
    {
        return x >= 0 && x < 8 && y >= 0 && y < 8;
    }

    bool isEmpty(int x, int y)
    {
        return board[y][x].TroopType == Troops::None;
    }

    bool isOpponentPiece(int x, int y, Color currentPlayerColor)
    {

        if (!isInsideBoard(x, y) || isEmpty(x, y))
        {
            return false;
        }

        return board[y][x].color != currentPlayerColor;
    }

    bool isFriendlyPiece(int x, int y, Color currentPlayerColor)
    {
        if (!isInsideBoard(x, y) || isEmpty(x, y))
        {
            return false;
        }

        return board[y][x].color == currentPlayerColor;
    }

    std::vector<std::pair<int, int>> getLegalMoves(Piece piece, int x, int y)
    {
        std::vector<std::pair<int, int>> moves;
        if (piece.TroopType == Troops::Pawn)
        {
            int direction = (piece.color == Color::Black) ? 1 : -1;
            int startRow = (piece.color == Color::Black) ? 1 : 6;

            if (isInsideBoard(x, y + direction) && isEmpty(x, y + direction))
            {
                moves.push_back({x, y + direction});

                if (y == startRow && isInsideBoard(x, y + 2 * direction) && isEmpty(x, y + 2 * direction))
                {
                    moves.push_back({x, y + 2 * direction});
                }
            }

            // Diagonal attack
            if (isInsideBoard(x - 1, y + direction) && isOpponentPiece(x - 1, y + direction, piece.color))
            {
                moves.push_back({x - 1, y + direction});
            }
            if (isInsideBoard(x + 1, y + direction) && isOpponentPiece(x + 1, y + direction, piece.color))
            {
                moves.push_back({x + 1, y + direction});
            }
        }
        else if (piece.TroopType == Troops::Knight)
        {
            std::vector<std::pair<int, int>> knightMoves = {
                {2, 1}, {2, -1}, {-2, 1}, {-2, -1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};

            for (auto move : knightMoves)
            {
                int newX = x + move.first;
                int newY = y + move.second;

                if (isInsideBoard(newX, newY) && !isFriendlyPiece(newX, newY, piece.color))
                {
                    moves.push_back({newX, newY});
                }
            }
        }
        else if (piece.TroopType == Troops::Rook)
        {

            std::vector<std::pair<int, int>> directions = {
                {0, 1},  // Move up (positive y)
                {0, -1}, // Move down (negative y)
                {1, 0},  // Move right (positive x)
                {-1, 0}  // Move left (negative x)
            };


            for (const auto &direction : directions)
            {
                int dx = direction.first;  // Change in x
                int dy = direction.second; // Change in y

                for (int j = 1; j < 8; j++) // Start from 1 to avoid the current square
                {
                    int newX = x + j * dx; 
                    int newY = y + j * dy; 

                    if (isInsideBoard(newX, newY))
                    {
                        if (isEmpty(newX, newY)) 
                        {
                            moves.push_back({newX, newY});
                        }
                        else if (isOpponentPiece(newX, newY, piece.color)) 
                        {
                            moves.push_back({newX, newY});
                            break; 
                        }
                        else 
                        {
                            break;
                        }
                    }
                    else
                    {
                        break; // Out of bounds, stop 
                    }
                }
            }
        }
        else if (piece.TroopType == Troops::Bishop)
        {
            std::vector<std::pair<int, int>> directions = {
                {1, 1},
                {1, -1},
                {-1, 1},
                {-1, -1}};


            for (const auto &direction : directions)
            {
                int dx = direction.first;  
                int dy = direction.second;

                for (int j = 1; j < 8; j++) 
                {
                    int newX = x + j * dx; 
                    int newY = y + j * dy;

                    if (isInsideBoard(newX, newY)) 
                    {
                        if (isEmpty(newX, newY)) 
                        {
                            moves.push_back({newX, newY});
                        }
                        else if (isOpponentPiece(newX, newY, piece.color)) 
                        {
                            moves.push_back({newX, newY});
                            break; 
                        }
                        else 
                        {
                            break;
                        }
                    }
                    else
                    {
                        break; // Out of boundss
                    }
                }
            }
        }
        else if (piece.TroopType == Troops::Queen)
        {
            std::vector<std::pair<int, int>> directions = {
                // combining both the bishop and the rook 's tactics
                {0, 1},
                {0, -1},
                {1, 0},
                {-1, 0},
                {1, 1},
                {1, -1},
                {-1, 1},
                {-1, -1}};


            for (const auto &direction : directions)
            {
                int dx = direction.first;  
                int dy = direction.second; 

                for (int j = 1; j < 8; j++) 
                {
                    int newX = x + j * dx; 
                    int newY = y + j * dy; 

                    if (isInsideBoard(newX, newY)) 
                    {
                        if (isEmpty(newX, newY)) 
                        {
                            moves.push_back({newX, newY});
                        }
                        else if (isOpponentPiece(newX, newY, piece.color)) 
                        {
                            moves.push_back({newX, newY});
                            break; 
                        }
                        else 
                        {
                            break;
                        }
                    }
                    else
                    {
                        break; // Out of bounds
                    }
                }
            }
        }
        else if (piece.TroopType == Troops::King)
        {
            std::vector<std::pair<int, int>> directions = {
                {0, 1},  // Move up
                {0, -1}, // Move down
                {1, 0},  // Move right
                {-1, 0}, // Move left
                {1, 1},  // Move up-right
                {1, -1}, // Move down-right
                {-1, 1}, // Move up-left
                {-1, -1} // Move down-left
            };


            for (const auto &direction : directions)
            {
                int dx = direction.first; 
                int dy = direction.second; // Change in y

                int newX = x + dx; 
                int newY = y + dy; 

                if (isInsideBoard(newX, newY)) 
                {
                    if (isEmpty(newX, newY)) 
                    {
                        moves.push_back({newX, newY});
                    }
                    else if (isOpponentPiece(newX, newY, piece.color)) 
                    {
                        moves.push_back({newX, newY});
                    }
                }
            }
        }
        return moves;
    }

    bool isLegalMove(int x, int y)
    {
        for (const auto &move : highlightMove)
        {
            if (move.first == x && move.second == y)
            {
                return true;
            }
        }
        return false;
    }

    Piece &get_PieceAt(int x, int y)
    {
        return board[y][x];
    }

    void renderPiece(SDL_Renderer *render, const Piece &piece, int x, int y)
    {
        if (piece.TroopType == Troops::None)
            return;

        SDL_Texture *texture = nullptr;

        if (piece.color == Color::Black)
        {
            switch (piece.TroopType)
            {
            case Troops::Pawn:
                texture = pieceTextures[0];
                break;
            case Troops::Rook:
                texture = pieceTextures[1];
                break;
            case Troops::Knight:
                texture = pieceTextures[2];
                break;
            case Troops::Bishop:
                texture = pieceTextures[3];
                break;
            case Troops::Queen:
                texture = pieceTextures[4];
                break;
            case Troops::King:
                texture = pieceTextures[5];
                break;
            default:
                std::cerr << "Error: Unknown troop type for white piece" << std::endl;
                break;
            }
        }
        else if (piece.color == Color::White)
        {
            switch (piece.TroopType)
            {
            case Troops::Pawn:
                texture = pieceTextures[6];
                break;
            case Troops::Rook:
                texture = pieceTextures[7];
                break;
            case Troops::Knight:
                texture = pieceTextures[8];
                break;
            case Troops::Bishop:
                texture = pieceTextures[9];
                break;
            case Troops::Queen:
                texture = pieceTextures[10];
                break;
            case Troops::King:
                texture = pieceTextures[11];
                break;
            default:
                std::cerr << "Error: Unknown troop type for balck piece" << std::endl;
                break;
            }
        }

        if (texture != nullptr)
        {
            SDL_Rect destRect = {x * 90 + 10, y * 90 + 10, 70, 70}; 
            SDL_RenderCopy(render, texture, nullptr, &destRect);
        }
        else
        {
            std::cerr << "Error: Null texture for piece at " << x << ", " << y << std::endl;
        }
    }

    void SetupBoard()
    {
        // for upper set  of pieces // black side
        board[0][0] = Piece(Troops::Rook, Color::Black);
        board[0][1] = Piece(Troops::Knight, Color::Black);
        board[0][2] = Piece(Troops::Bishop, Color::Black);
        board[0][3] = Piece(Troops::Queen, Color::Black);
        board[0][4] = Piece(Troops::King, Color::Black);
        board[0][5] = Piece(Troops::Bishop, Color::Black);
        board[0][6] = Piece(Troops::Knight, Color::Black);
        board[0][7] = Piece(Troops::Rook, Color::Black);

        for (int j = 0; j < 8; j++)
        {
            board[1][j] = Piece(Troops::Pawn, Color::Black); 
        }

        // for lower set  of pieces  // white side
        board[7][0] = Piece(Troops::Rook, Color::White);
        board[7][1] = Piece(Troops::Knight, Color::White);
        board[7][2] = Piece(Troops::Bishop, Color::White);
        board[7][3] = Piece(Troops::Queen, Color::White);
        board[7][4] = Piece(Troops::King, Color::White);
        board[7][5] = Piece(Troops::Bishop, Color::White);
        board[7][6] = Piece(Troops::Knight, Color::White);
        board[7][7] = Piece(Troops::Rook, Color::White);

        for (int j = 0; j < 8; j++)
        {
            board[6][j] = Piece(Troops::Pawn, Color::White); 
        }
    }

    char get_PieceAtdata(const Piece &pieces)
    {
        if (pieces.color == Color::Black)
        {
            switch (pieces.TroopType)
            {
            case Troops::Rook:
                return 'R';
            case Troops::Pawn:
                return 'P';
            case Troops::Knight:
                return 'N';
            case Troops::Bishop:
                return 'B';
            case Troops::Queen:
                return 'Q';
            case Troops::King:
                return 'K';
            default:
                return '.';
            }
        }

        else if (pieces.color == Color::White)
        {
            switch ((pieces.TroopType))
            {

            case Troops::Rook:
                return 'r';
            case Troops::Pawn:
                return 'p';
            case Troops::Knight:
                return 'n';
            case Troops::Bishop:
                return 'b';
            case Troops::Queen:
                return 'q';
            case Troops::King:
                return 'k';
            default:
                return '.';
            }
        }

        return '.';
    }

    void render(SDL_Renderer *renderer)
    {
        SDL_Rect boardRect;
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                boardRect = {i * 90, j * 90, 90, 90};

                if ((i + j) % 2 == 0)
{
    SDL_SetRenderDrawColor(renderer, 0xee, 0xee, 0xee, 255); // Lighter gray for light squares
}
else
{
    SDL_SetRenderDrawColor(renderer, 0x33, 0x33, 0x33, 255); // Darker gray for dark squares
}
SDL_RenderFillRect(renderer, &boardRect);

if (selectedPiece != std::make_pair(-1, -1) && selectedPiece == std::make_pair(i, j))
{
    SDL_SetRenderDrawColor(renderer, 0x66, 0xcc, 0xff, 255); // Light blue for selected piece
    SDL_RenderFillRect(renderer, &boardRect);
}

for (auto move : highlightMove)
{
    if (move.first == i && move.second == j)
    {
        if ((i + j) % 2 == 0)
        {
            SDL_SetRenderDrawColor(renderer, 0x7c, 0xfc, 0x00, 128); // Semi- green for valid moves on light squares
        }
        else
        {
            SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xff, 128); // Semi- blue for valid moves on dark squares
        }
        SDL_RenderFillRect(renderer, &boardRect);
    }
}


                renderPiece(renderer, get_PieceAt(i, j), i, j);
            }
        }
    }
    std::pair<int, int> getSelectedPiece() const
    {
        return selectedPiece;
    }


    void setSelectedPiece(int x, int y)
    {
        selectedPiece = std::make_pair(x, y);
    }

    void promotePawnSDL(SDL_Renderer *renderer, int x, int y, Color color)
    {

        SDL_Rect promotionOptions[4];
        int optionWidth = 100;
        int optionHeight = 100;


        promotionOptions[0] = {100, 200, optionWidth, optionHeight}; // Queen
        promotionOptions[1] = {250, 200, optionWidth, optionHeight}; // Rook
        promotionOptions[2] = {400, 200, optionWidth, optionHeight}; // Bishop
        promotionOptions[3] = {550, 200, optionWidth, optionHeight}; // Knight


        SDL_Texture *rookTexture = nullptr;
        SDL_Texture *knightTexture = nullptr;
        SDL_Texture *bishopTexture = nullptr;
        SDL_Texture *queenTexture = nullptr;

        if (color == Color::White)
        {
            queenTexture = IMG_LoadTexture(renderer, "textures/White_Queen.png");
            rookTexture = IMG_LoadTexture(renderer, "textures/White_Rook.png");
            knightTexture = IMG_LoadTexture(renderer, "textures/White_Knight.png");
            bishopTexture = IMG_LoadTexture(renderer, "textures/White_Bishop.png");
        }
        else if (color == Color::Black)
        {
            queenTexture = IMG_LoadTexture(renderer, "textures/Black_Queen.png");
            rookTexture = IMG_LoadTexture(renderer, "textures/Black_Rook.png");
            knightTexture = IMG_LoadTexture(renderer, "textures/Black_Knight.png");
            bishopTexture = IMG_LoadTexture(renderer, "textures/Black_Bishop.png");
        }


        if (!queenTexture || !rookTexture || !bishopTexture || !knightTexture)
        {
            std::cerr << "Error loading textures!" << std::endl;
            return;
        }

        bool promotionSelected = false;
        while (!promotionSelected)
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                if (event.type == SDL_MOUSEBUTTONDOWN)
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    SDL_Point mousePoint = {mouseX, mouseY};

                    if (SDL_PointInRect(&mousePoint, &promotionOptions[0]))
                    {
                        board[y][x] = Piece(Troops::Queen, color); // Promote to Queen
                        promotionSelected = true;
                    }
                    else if (SDL_PointInRect(&mousePoint, &promotionOptions[1]))
                    {
                        board[y][x] = Piece(Troops::Rook, color); // Promote to Rook
                        promotionSelected = true;
                    }
                    else if (SDL_PointInRect(&mousePoint, &promotionOptions[2]))
                    {
                        board[y][x] = Piece(Troops::Bishop, color); // Promote to Bishop
                        promotionSelected = true;
                    }
                    else if (SDL_PointInRect(&mousePoint, &promotionOptions[3]))
                    {
                        board[y][x] = Piece(Troops::Knight, color); // Promote to Knight
                        promotionSelected = true;
                    }
                }
            }


            SDL_RenderClear(renderer);


            SDL_RenderCopy(renderer, queenTexture, nullptr, &promotionOptions[0]);
            SDL_RenderCopy(renderer, rookTexture, nullptr, &promotionOptions[1]);
            SDL_RenderCopy(renderer, bishopTexture, nullptr, &promotionOptions[2]);
            SDL_RenderCopy(renderer, knightTexture, nullptr, &promotionOptions[3]);

            SDL_RenderPresent(renderer);
        }

        // Clean up textures
        SDL_DestroyTexture(queenTexture);
        SDL_DestroyTexture(rookTexture);
        SDL_DestroyTexture(bishopTexture);
        SDL_DestroyTexture(knightTexture);
    }

    bool movePiece(int srcX, int srcY, int destX, int destY)
    {
        // Ensure the source and destination are inside the board
        if (!isInsideBoard(srcX, srcY) || !isInsideBoard(destX, destY))
        {
            return false;
        }

        Piece backUpPiece = board[destY][destX];
        Piece pieceToMove = board[srcY][srcX];

        if (pieceToMove.TroopType == Troops::Pawn)
        {
            if ((pieceToMove.color == Color::White && destY == 0) ||
                (pieceToMove.color == Color::Black && destY == 7))
            {
                board[destY][destX] = pieceToMove;

                promotePawnSDL(renderer, destX, destY, pieceToMove.color);
                Promotion_Sound.play(1);

                // Clear original pawn position after promotion
                board[srcY][srcX] = Piece(Troops::None, Color::None);
                return true;
            }
        }

        // Move the piece from (srcX, srcY) to (destX, destY)
        board[destY][destX] = pieceToMove;                    
        board[srcY][srcX] = Piece(Troops::None, Color::None); 


        std::pair<int, int> kingPosition = findKingPosition(pieceToMove.color);

        if (IsKingCheck(kingPosition.first, kingPosition.second, pieceToMove.color))
        {

            board[srcY][srcX] = pieceToMove;
            board[destY][destX] = backUpPiece;
            return false;
        }

        if (backUpPiece.TroopType != Troops::None)
        {
            attack_Sound.play(1);
        }
        else
        {
            move_Sound.play(1);
        }


        return true;
    }

    std::pair<int, int> findKingPosition(Color kingColor)
    {
        for (int x = 0; x < 8; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                if (board[y][x].TroopType == Troops::King && board[y][x].color == kingColor)
                {
                    return {x, y};
                }
            }
        }
        return {-1, -1};
    }

    bool IsKingCheck(int kx, int ky, Color kingColor)
    {
        for (int i = 0; i < 8; i++)
        {
            for (int j = 0; j < 8; j++)
            {
                Piece &piece = board[j][i];

                if (piece.color != kingColor && piece.TroopType != Troops::None)
                {
                    std::vector<std::pair<int, int>> LegalMoves = getLegalMoves(piece, i, j);

                    for (const auto &move : LegalMoves)
                    {
                        if (move.first == kx && move.second == ky)
                        {
                            kingcheck_Sound.play(1);
                            std::cout << "king is in check" << std::endl;
                            return true; // The King is in check
                        }
                    }
                }
            }
        }
        return false;
    }
};

void RenderText(SDL_Renderer *renderer, const std::string &message, int x, int y, TTF_Font *font)
{
    SDL_Color color = {0x00, 0x00, 0x00}; // White color // 5ab787
    SDL_Surface *surfaceMessage = TTF_RenderText_Solid(font, message.c_str(), color);
    SDL_Texture *messageTexture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);

    SDL_Rect messageRect;
    messageRect.x = x;
    messageRect.y = y;
    messageRect.w = surfaceMessage->w;
    messageRect.h = surfaceMessage->h;

    SDL_RenderCopy(renderer, messageTexture, nullptr, &messageRect);

    SDL_FreeSurface(surfaceMessage);
    SDL_DestroyTexture(messageTexture);
}

int main()
{

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << " system not initlalised" << std::endl;
        return -1;
    }

    SDL_Window *window = SDL_CreateWindow("Chess", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cout << "Window could not be created: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (TTF_Init() == -1)
    {
        std::cout << "Failed to initialize TTF: " << TTF_GetError() << std::endl;
        return -1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cout << "Renderer could not be created: " << SDL_GetError() << std::endl;
        return -1;
    }

    if (IMG_Init(IMG_INIT_PNG) == 0)
    {
        std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
        return -1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cout << "sdl autio no initialsed " << std::endl;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
    {
        std::cerr << "SDL_mixer could not initialize! SDL_mixer Error: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    TTF_Font *Start_Screen = TTF_OpenFont("Font/LIVINGBY.TTF", 200);
    if (Start_Screen == nullptr)
    {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    TTF_Font *Over_Screen = TTF_OpenFont("Font/LIVINGBY.TTF", 200);
    if (Over_Screen == nullptr)
    {
        std::cout << "Failed to load font: " << TTF_GetError() << std::endl;
    }

    move_Sound.Load("Sound/move-self.wav");
    attack_Sound.Load("Sound/capture.wav");
    checkmate_Sound.Load("Sound/game-end.wav");
    kingcheck_Sound.Load("Sound/move-check.wav");
    gamestart_Sound.Load("Sound/game-start.wav");
    Promotion_Sound.Load("Sound/capture.wav");

    SDL_Surface *startscreen = SDL_LoadBMP("images/start_Screen.bmp");
    SDL_Texture *start_texture = SDL_CreateTextureFromSurface(renderer, startscreen);
    SDL_FreeSurface(startscreen);

    SDL_Surface *GameOver_surface = SDL_LoadBMP("images/GameOver_Screen.bmp");
    SDL_Texture *GameOver_texture = SDL_CreateTextureFromSurface(renderer, GameOver_surface);
    SDL_FreeSurface(GameOver_surface);

    ChessBoard chessboard(renderer);
    Color currentPlayerColor = Color::White;
    Piece *selectedPiece = nullptr;
    chessboard.printBoard();
    GameState gamestate = STARTINGSCREEN;
    bool isCheckmate = false;
    bool isStalemate = false;
    std::string winner;

    bool IsGameRunning = true;

    while (IsGameRunning)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                IsGameRunning = false;
            }
            if (gamestate == STARTINGSCREEN)
            {
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN)
                {
                    gamestart_Sound.play(1);
                    gamestate = PLAYING;
                }
            }
            if (gamestate == GAMEOVER)
            {
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_q)
                {
                    IsGameRunning = false;
                }
            }
            if (gamestate == PLAYING)
            {
                if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    int boardX = mouseX / 90; // this be give the column
                    int boardY = mouseY / 90; // this be give the row

                    Piece &target_PieceAt = chessboard.get_PieceAt(boardX, boardY);

                    if (chessboard.isPieceSelected()) // If a piece is already selected
                    {
                        // Get the currently selected piece
                        std::pair<int, int> selectedPiece = chessboard.getSelectedPiece();

                        // Check if clicked position is a legal move
                        if (chessboard.isLegalMove(boardX, boardY))
                        {
                            // Move the selected piece to the new position
                            bool movesuccess = chessboard.movePiece(selectedPiece.first, selectedPiece.second, boardX, boardY);

                            // After moving, deselect the piece and remove highlights
                            if (movesuccess)
                            {
                                currentPlayerColor = (currentPlayerColor == Color::Black) ? Color::White : Color::Black;

                                if (chessboard.isCheckMate(currentPlayerColor))
                                {
                                    std::cout << "Checkmate !!" << ((currentPlayerColor == Color::White) ? "Black" : "White") << " wins!" << std::endl;
                                    isCheckmate = true;
                                    winner = (currentPlayerColor == Color::White) ? "Black" : "White";
                                    checkmate_Sound.play(1);
                                    gamestate = GAMEOVER;
                                    SDL_Delay(35);
                                }
                                else if (chessboard.isStaleMate(currentPlayerColor))
                                {
                                    std::cout << "Stalemate! It's a draw." << std::endl;
                                    isStalemate = true;
                                    checkmate_Sound.play(1);
                                    gamestate = GAMEOVER;
                                    SDL_Delay(35);
                                }
                                chessboard.deselectPiece();
                            }
                            else
                            {
                                chessboard.deselectPiece();
                            }
                        }
                        else
                        {
                            // Deselect the piece if clicked on an invalid square
                            chessboard.deselectPiece();
                        }
                    }
                    else // If no piece is selected
                    {
                        Piece &piece = chessboard.get_PieceAt(boardX, boardY); // Get the clicked piece

                        // Check if the clicked square contains a valid piece (i.e., current player's piece)
                        if (piece.TroopType != Troops::None && piece.color == currentPlayerColor)
                        {
                            chessboard.selectPiece(boardX, boardY);
                        }
                    }
                }
            }
        }

        switch (gamestate)
        {
        case GameState::STARTINGSCREEN:
        {
            SDL_RenderClear(renderer);
            SDL_Rect rect = {0, 0, 720, 720};
            SDL_RenderCopy(renderer, start_texture, nullptr, &rect);
            RenderText(renderer, "Chess !!", 50, 50, Start_Screen);
            SDL_RenderPresent(renderer);
            break;
        }
        case GameState::PLAYING:
            SDL_RenderClear(renderer);
            chessboard.render(renderer);
            SDL_RenderPresent(renderer);
            break;
        case GameState::GAMEOVER:
        {
            SDL_RenderClear(renderer);
            SDL_Rect rect = {0, 0, 720, 720};
            SDL_RenderCopy(renderer, GameOver_texture, nullptr, &rect);
            RenderText(renderer, winner, 50, 50, Over_Screen);
            RenderText(renderer, "Wins !!", 100, 250, Over_Screen);
            SDL_RenderPresent(renderer);
            break;
        }
        }

        SDL_Delay(16);
    }
    selectedPiece = nullptr;
    delete selectedPiece;

    SDL_DestroyTexture(GameOver_texture);
    SDL_DestroyTexture(start_texture);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
