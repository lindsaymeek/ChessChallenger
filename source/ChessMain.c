#include "drivers.h"
//---------------------------------------------------------------------------

typedef unsigned char	Byte;
typedef unsigned char	BOOL;

typedef	enum
{
	FALSE,
	TRUE
};

typedef BOOL        	TSide;

typedef struct
{
    Byte    x1;
    Byte    x2;
    Byte    y1;
    Byte    y2;
    Byte    depth;
    TSide   side;
    BOOL    ok;
} TChess;

// Piece & Colour definitions
#define	PAWN		1
#define	KNIGHT		2
#define	BISHOP		3
#define	ROOK		4
#define	QUEEN		5
#define	KING		6

#define	NO_PIECE	0
#define WHITE       1
#define BLACK       0

#define	W_PAWN		PAWN + (WHITE<<3)
#define W_ROOK		ROOK + (WHITE<<3)
#define W_KNIGHT	KNIGHT + (WHITE<<3)
#define W_BISHOP	BISHOP + (WHITE<<3)
#define W_QUEEN		QUEEN + (WHITE<<3)
#define W_KING		KING + (WHITE<<3)
#define	B_PAWN		PAWN + (BLACK<<3)
#define B_ROOK		ROOK + (BLACK<<3)
#define B_KNIGHT	KNIGHT + (BLACK<<3)
#define B_BISHOP	BISHOP + (BLACK<<3)
#define B_QUEEN		QUEEN + (BLACK<<3)
#define B_KING		KING + (BLACK<<3)

#define SQUARE_NONE 255

#define WHTLED      0x01    // white status led
#define BLKLED      0x02    // black status led
#define CHKLED      0x04    // check status led
#define MTELED      0x08    // mate status led

#define MAXLK       6       // Maximum search depth
#define	MAX			(MAXLK+1)

/*
	Within each nibble of the board memory, the highest bit in a nibble
	represents the piece colour as defined above.
*/

static const    Byte	PieceSet[64] =
{
	W_ROOK,W_KNIGHT,W_BISHOP,W_QUEEN,W_KING,W_BISHOP,W_KNIGHT,W_ROOK,
	W_PAWN,W_PAWN,W_PAWN,W_PAWN,W_PAWN,W_PAWN,W_PAWN,W_PAWN,
	NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,
    NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,
	NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,
	NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,NO_PIECE,
	B_PAWN,B_PAWN,B_PAWN,B_PAWN,B_PAWN,B_PAWN,B_PAWN,B_PAWN,
	B_ROOK,B_KNIGHT,B_BISHOP,B_QUEEN,B_KING,B_BISHOP,B_KNIGHT,B_ROOK
};

static  const   Byte    PieceValue[7] = { 0,2,6,6,10,18,40 };

//---------------------------------------------------------------------------
// General Set and Look Up piece functions
static	void	SetUpBoard(void);
static	void	SetPiece(Byte square,Byte piece);
static	Byte	GetPiece(Byte square);
static	Byte	PieceDefn(Byte square);
static	Byte	Col(Byte square);
static	Byte	Row(Byte square);
static	Byte	AbsDiff(Byte x,Byte y);
// Chess algorithm functions
static	Byte	BdLkup(Byte col,Byte row);
static	void	MoveGen(Byte depth,Byte piece);
static  Byte    Bounds(Byte depth,Byte pawnf);
static  Byte    Move(Byte depth,Byte msave);
static  void    MoveF(Byte depth,Byte msave,Byte mxy,Byte mpq);
static  void    MoveB(Byte depth,Byte msave,Byte mxy,Byte mpq);
static  void    Bishop(Byte depth);
static  void    Knight(Byte depth);
static  void    Rook(Byte depth);
static  void    King(Byte depth);
static  void    Pawn(Byte depth);
static  Byte    FindKing(TSide side);
static  BOOL    IsKingInCheck(TSide side);
static  void    CheckMove(TChess* pChess);
static  Byte    MinMax(Byte depth);
static  void    DoChess(TChess* pChess);
static	void	CheckPromotePawn(Byte to,TSide side);
static	void	CheckPerformCastle(Byte from,Byte to,TSide side);
static	void 	ShowMoveLCD(Byte from,Byte to,Byte piece);

//---------------------------------------------------------------------------

static	Byte	ChessBoard[32];
static	Byte	BestScore[MAX];
static	Byte	BestX[MAX];
static	Byte	BestY[MAX];
static	Byte	BestP[MAX];
static	Byte	BestQ[MAX];
static	Byte	MoveOk[MAX];
static	TSide	Side[MAX];

static	Byte	t1,t2,t3,t4,function,pieceValue,x,y,p,q,kingF,ok,castle;
static	Byte	x1,x2,y1,y2,forcePlay,mySide,mate,status,PlayLevel,PruneTree;
static	TSide	side = WHITE;

#define	STATUS	(((side==WHITE)?CHESS_WHITE:CHESS_BLACK)+(IsKingInCheck(side)?CHESS_CHECK:0)+mate)

//---------------------------------------------------------------------------

void	ChessMain(void)
{
	Byte	from,to,capPiece,sq;
	TChess	chess;

	SetUpBoard();
	while (1)
	{
		// Display Status
		status = STATUS;
		Chess_LedStat(status);
		// Test for Game end?
		if (mate==CHESS_MATE)
			continue;
		// Is this my Opponents Move?
		if (side==mySide)
		{
			// Get Opponents move
			from = Chess_ScanBoard(-1,STATUS);
			to = Chess_ScanBoard(from,STATUS);
			//Check for user correction
			ShowMoveLCD(from,to,GetPiece(from));
			if (from!=to)
			{
				chess.x1 = Col(from);
				chess.y1 = Row(from);
				chess.x2 = Col(to);
				chess.y2 = Row(to);
				chess.depth = 0;
				DoChess(&chess);
				if (chess.ok)
				{
					// Move Valid...well update Chessboard
					capPiece = GetPiece(to);
					SetPiece(to,GetPiece(from));
					SetPiece(from,NO_PIECE);
					if (IsKingInCheck(side))
					{
						// Invalid...can't move into check
						SetPiece(from,GetPiece(to));
						SetPiece(to,capPiece);
						// InValid Move....tell user to put back piece
						do {
							Chess_Beep();
							Chess_Beep();
							to = Chess_ScanBoard(from,STATUS);
				   		} while (to!=from);
					}
					else
					{
						CheckPromotePawn(to,side);
						CheckPerformCastle(from,to,side);
						side ^= 1;	// Swap sides
					}
				}
				else
				{
					// InValid Move....tell user to put back piece
					do {
						Chess_Beep();
						Chess_Beep();
						to = Chess_ScanBoard(from,STATUS);
				   	} while (to!=from);
				}
			}
		}
		// It is the Computers Move.
		else	
		{
			chess.depth = PlayLevel;
			chess.side = side;
			DoChess(&chess);
			from = ((chess.y1*8)+chess.x1);
			to = ((chess.y2*8)+chess.x2);
			if (MoveOk[chess.depth])
			{
				// Show Computers Move
				ShowMoveLCD(from,to,GetPiece(from));
				mate = 0;
				// Do Computers Move
				Chess_Beep();
				for (sq=Chess_ScanBoard(from,STATUS);sq!=from;)
				{
					Chess_Beep();
					Chess_Beep();
					sq = Chess_ScanBoard(from,STATUS);
				}
				for (sq=Chess_ScanBoard(to,STATUS);sq!=to;)
				{
					Chess_Beep();
					Chess_Beep();
					sq = Chess_ScanBoard(to,STATUS);
				}
				// Make the Move on the ChessBoard
				SetPiece(to,GetPiece(from));
				SetPiece(from,NO_PIECE);
				// Test for PAWN promotion and castling
				CheckPromotePawn(to,side);
				CheckPerformCastle(from,to,side);
				side ^= 1;	//swap sides
				// Is opponents KING in check ... assume NO
				kingF = FALSE;
				if (IsKingInCheck(side))
				{
					// KING is in check...indicate it.
					kingF = TRUE;
					// Can the opponents KING move?
					from = ((chess.y2*8)+chess.x2);
					// Assume it can't
					mate = CHESS_MATE;
					for (to=0; to<64; to++)
					{
						chess.x1 = Col(from);
						chess.y1 = Row(from);
						chess.x2 = Col(to);
						chess.y2 = Row(to);
						chess.depth = 0;
						DoChess(&chess);
						if (chess.ok)
						{
							capPiece = GetPiece(to);
							SetPiece(to,GetPiece(from));
							SetPiece(from,NO_PIECE);
							if (!IsKingInCheck(side))
								mate = 0; 	// KING can Move!
							SetPiece(from,GetPiece(to));
							SetPiece(to,capPiece);
						}
						if (!mate)
							break;
					}
				}
			}
			else
				mate = CHESS_MATE;	
		}
	}
}
//---------------------------------------------------------------------------
static	void	SetUpBoard(void)
{
	Byte	i,x;

	// Initialise some globals
	mate = 0;
	castle = 0xFF;
	PlayLevel = 1;
	PruneTree = 0;
	// Initialise the Chess Board
	for (i=0,x=0; i<64; i+=2,x++)
		ChessBoard[x] = (PieceSet[i]<<4)+PieceSet[i+1];
	// Select PlayLevel
	LCD_clear();
	LCD_at(0,0);
	LCD_str("Play Level  ");
	LCD_at(12,0);
	LCD_char(PlayLevel+'0');
	LCD_at(0,1);
	LCD_str(" OK   -   +");
	x=1;
	i=0;
	while(x)
	{
		if (ScanButton(i))
		{
			switch (i)
			{
			case 0:	
				if (PlayLevel<MAXLK)
					PlayLevel++;
				break;
			case 1:
				if (PlayLevel>1)
					PlayLevel--;
				break;
			case 2:
				x = 0;
				break;
			}
			while (ScanButton(i));
			LCD_at(12,0);
			LCD_char(PlayLevel+'0');
		}
		if (++i==4)
			i = 0;
	}
	while(ScanButton(0) || ScanButton(1) || ScanButton(2) || ScanButton(3));
	// Select Play Side
	LCD_clear();
	LCD_at(0,0);
	LCD_str("Play White = B3");
	LCD_at(0,1);
	LCD_str("Play Black = B0");
	while(!ScanButton(3) && !ScanButton(0));
	LCD_clear();
	mySide = (ScanButton(3))?WHITE:BLACK;
	// Shall we Prune the tree
	while(ScanButton(0) || ScanButton(1) || ScanButton(2) || ScanButton(3));
	PruneTree = (PlayLevel>4)?TRUE:FALSE;
}
//---------------------------------------------------------------------------
static	void	SetPiece(Byte square,Byte piece)
{
	Byte*	pPos;

	pPos = &ChessBoard[square/2];
	if ((square & 0x01)==0)
		*pPos = (*pPos&0x0F)|(piece<<4);
	else
		*pPos = (*pPos&0xF0)|piece;
}
//---------------------------------------------------------------------------
static	Byte	GetPiece(Byte square)
{
	Byte* 	pPos;

	pPos = &ChessBoard[square/2];
	if ((square & 0x01)==0)
		return	((*pPos>>4)&0x0F);
	return	(*pPos&0x0F);
}
//---------------------------------------------------------------------------
static	Byte	PieceDefn(Byte square)
{
	return	(GetPiece(square)&0x07);
}
//---------------------------------------------------------------------------
static	Byte	Col(Byte square)
{
	return	(square%8);
}
//---------------------------------------------------------------------------
static	Byte	Row(Byte square)
{
	return	(square/8);
}
//---------------------------------------------------------------------------
static	Byte	AbsDiff(Byte x,Byte y)
{
	if (x>=y)
		return	(x-y);
	return	(y-x);
}
//---------------------------------------------------------------------------
static	Byte	BdLkup(Byte col,Byte row)
{
	return	GetPiece((row*8)+col);
}
//---------------------------------------------------------------------------
static	void	MoveGen(Byte depth,Byte piece)
{
    switch(piece & 0x07)
    {
    case PAWN:      Pawn(depth);    break;
    case BISHOP:    Bishop(depth);  break;
    case KNIGHT:    Knight(depth);  break;
    case ROOK:      Rook(depth);    break;
    case QUEEN:     Rook(depth);    Bishop(depth);  break;
    case KING:      King(depth);    break;
    }
}
//---------------------------------------------------------------------------
static  Byte    Bounds(Byte depth,Byte pawnf)
{
    Byte    piece,side;

    // Check if off board
    if (p>7 || q>7)
        return  2;
    // Get a piece and it's side
    piece = BdLkup(p,q);
    side = (piece>>3);
    piece = (piece & 0x07);
    // Return normal stops
    if (pawnf==2)
    {
        if (piece==NO_PIECE)    return  0;
        if (side==Side[depth])  return  2;
        return  1;
    }
    else if (pawnf>2)   return  pawnf;
    //We have a pawn!
    if (pawnf==0 && piece==NO_PIECE)    return  0;
    if (pawnf==1 && side!=Side[depth] && piece!=NO_PIECE)   return  1;
    return  2;
}
//---------------------------------------------------------------------------
static  Byte    Move(Byte depth,Byte msave)
{
    Byte    mxy,mpq;

    msave = Bounds(depth,msave);    // msave doubles as pawn flag
    if (msave==2)   return  2;      // stop
    msave = msave<<4;               // save stop in upper nibble
    switch(function)
    {
    case 0: // Just checking if a move is valid
        if (p==x2 && q==y2) ok = TRUE;
        break;
    default:    // Move a piece
        msave = msave | BdLkup(p,q);
        mxy = ((y*8)+x);
        mpq = ((q*8)+p);
        MoveF(depth,msave,mxy,mpq);
        // Don't move if in check
//        if (!IsKingInCheck(Side[depth]))
            t1 = MinMax(depth-1);       // Recursive call
//        else if (!t1)
//            t1 = (Side[depth]==WHITE)?1:255;
        MoveB(depth,msave,mxy,mpq);
        x = Col(mxy);
        y = Row(mxy);
        p = Col(mpq);
        q = Row(mpq);
        // Detect best move
        t2 = 0;
        t3 = BestScore[depth];
        t4 = Side[depth];
        if (t4==WHITE && t1>t3) t2 = 1; // Best for WHITE
        if (t4==BLACK && t1<t3) t2 = 1; // Best for BLACK
        if (t2)// && t1!=255 && t1!=1)
        {
            BestScore[depth] = t1;
            BestX[depth] = x;
            BestY[depth] = y;
            BestP[depth] = p;
            BestQ[depth] = q;
            MoveOk[depth] = TRUE;
        }
        break;
    }
    return  (msave>>4);
}
//---------------------------------------------------------------------------
static  void    MoveF(Byte depth,Byte msave,Byte mxy,Byte mpq)
{
    t2 = msave & 0x0F;      // piece taken
    t3 = (t2>>3);
    t4 = PieceValue[t2 & 0x07];
    if ((t2 & 0x07)==KING)  kingF = TRUE;
    // Was this a Castle?
    msave >>= 4;
    if (msave>=4)
    {
        SetPiece(((msave==4)?(mpq-1):(mpq+1)),GetPiece((msave==4)?(mpq+1):(mpq-2)));
        SetPiece(((msave==4)?(mpq+1):(mpq-2)),NO_PIECE);
    }
    // Subs zero for empty spaces
    if (t3)
        pieceValue -= t4;
    else
        pieceValue += t4;
    // Make the move
    msave = GetPiece(mxy);
	SetPiece(mpq,msave);
	SetPiece(mxy,NO_PIECE);
    // Prevent King from moving
    if ((msave & KING)==KING)
    {
        if ((msave>>3)==WHITE)
        {
            castle &= 0x7F;     // Indicate WHITE KING moved
            pieceValue -= 1;
        }
        else
        {
            castle &= 0xF7;     // Indicate BLACK KING moved
            pieceValue += 1;
        }
    }
}
//---------------------------------------------------------------------------
static  void    MoveB(Byte depth,Byte msave,Byte mxy,Byte mpq)
{
    t2 = msave & 0x0F;      // piece taken
    t3 = (t2>>3);
    t4 = PieceValue[t2 & 0x07];
    if ((t2 & 0x07)==KING)  kingF = FALSE;
    // Was this a Castle?
    msave >>= 4;
    if (msave>=4)
    {
        SetPiece(((msave==4)?(mpq+1):(mpq-2)),GetPiece((msave==4)?(mpq-1):(mpq+1)));
        SetPiece(((msave==4)?(mpq-1):(mpq+1)),NO_PIECE);
    }
    // Subs zero for empty spaces
    if (t3)
        pieceValue += t4;
    else
        pieceValue -= t4;
    // Make the move
    msave = GetPiece(mpq);
	SetPiece(mxy,msave);
	SetPiece(mpq,t2);
    // Prevent King from moving
    if ((msave & KING)==KING)
    {
        if ((msave>>3)==WHITE)
        {
            castle |= 0x80;
            pieceValue += 1;
        }
        else
        {
            castle |= 0x08;
            pieceValue -= 1;
        }
    }
}
//---------------------------------------------------------------------------
static  void    Bishop(Byte depth)
{
    Byte    stop;

    // Test NE direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p++; q++;
        stop = Move(depth,2);
    }
    // Test SE direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p++; q--;
        stop = Move(depth,2);
    }
    // Test NW direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p--; q++;
        stop = Move(depth,2);
    }
    // Test SW direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p--; q--;
        stop = Move(depth,2);
    }
}
//---------------------------------------------------------------------------
static  void    Knight(Byte depth)
{
    Byte    dy,inc;

    p = x-3; dy = 0; inc = 1;
    while (p!=(x+2))
    {
        dy += inc;
        p++;
        q = y + dy;
        if (p==x)
        {
            inc = 255;
            continue;
        }
        Move(depth,2);
        q = y - dy;
        Move(depth,2);
    }
}
//---------------------------------------------------------------------------
static  void    Rook(Byte depth)
{
    Byte    stop;

    // Test E direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p++;
        stop = Move(depth,2);
    }
    // Test W direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        p--;
        stop = Move(depth,2);
    }
    // Test N direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        q++;
        stop = Move(depth,2);
    }
    // Test S direction
    stop = 0; p = x; q = y;
    while (stop==0)
    {
        q--;
        stop = Move(depth,2);
    }
}
//---------------------------------------------------------------------------
static  void    King(Byte depth)
{
    Byte    sp,sq;

    sp = p;
    sq = q;
    // Test for castle ... ignore if done already
    if (x==4 && (y==0 || y==7) && kingF==0)
    {
        if (function==0)    // Is this a user move?
            p = x2;         // Yes
        p = ((p<x)?0x02:0x04);
        p = ((Side[depth]==WHITE)?(0x80|(p<<4)):(0x08|p));
        if ((castle & p)==p)
        {
            // Make sure squares are empty
            q = y;
            if ((BdLkup((x-1),q)==NO_PIECE) &&
                (BdLkup((x-2),q)==NO_PIECE) &&
                (BdLkup((x-3),q)==NO_PIECE))
            {
                // Test Queen side
                p = (x-2);
                Move(depth,8);
            }
            if (BdLkup((x+1),q)==NO_PIECE &&
                BdLkup((x+2),q)==NO_PIECE)
            {
                // Test King side
                p = (x+2);
                Move(depth,4);
            }
        }
    }
    sp = x+2;
    sq = y+2;
    // Test for normal move
    for (p=(x-1); p!=sp; p++)
    {
        for (q=(y-1); q!=sq; q++)
            Move(depth,2);
    }
}
//---------------------------------------------------------------------------
static  void    Pawn(Byte depth)
{
    Byte    pawnF,stop;

    stop = 0;
    pawnF = 1;  // Test captures first
    p = x+1; q = y;
    if (Side[depth]==WHITE)
    {
        q++;
        Move(depth,pawnF);
        p -= 2;
        Move(depth,pawnF);
        p = x; pawnF = 0;
        stop = Move(depth,pawnF);
        if (stop==0 && y==1)
        {
            q++;
            Move(depth,pawnF);
        }
    }
    else
    {
        q--;
        Move(depth,pawnF);
        p -= 2;
        Move(depth,pawnF);
        p = x; pawnF = 0;
        stop = Move(depth,pawnF);
        if (stop==0 && y==6)
        {
            q--;
            Move(depth,pawnF);
        }
    }
}
//---------------------------------------------------------------------------
static  Byte    FindKing(TSide side)
{
    Byte    square,piece;

    for (square=0; square<64; square++)
    {
        piece = BdLkup(Col(square),Row(square));
        if ((piece>>3)==side && (piece&7)==KING)
            break;
    }
    return  square;
}
//---------------------------------------------------------------------------
static  BOOL    IsKingInCheck(TSide side)
{
    ok = FALSE;
    // Make sure Side's King is NOT in check
    x2 = FindKing(side);
    // Make sure NOT in check
    function = 0;
    y2 = Row(x2);  x2 = Col(x2);
    for (x1=0; x1<64; x1++)
    {
        y1 = BdLkup(Col(x1),Row(x1));
        if (y1 && (y1>>3)!=side)
        {
            x = Col(x1);
            y = Row(x1);
            Side[0] = (side==WHITE)?BLACK:WHITE;
            MoveGen(0,y1);
            if (ok) // King in check then break!
                break;
        }
    }
    function = 2;
    return  (ok!=FALSE);
}
//---------------------------------------------------------------------------
static  void    CheckMove(TChess* pChess)
{
    Byte    piece;

    x = pChess->x1;
    y = pChess->y1;
    Side[pChess->depth] = (((piece=BdLkup(x,y))>>3)&WHITE);
    MoveGen(pChess->depth,piece);
}
//---------------------------------------------------------------------------
static  Byte    MinMax(Byte depth)
{
    Byte    i,piece;

    // Is this as deep we want to go OR has a king been checked?
	if (depth==0 || kingF)
        return  pieceValue;
    // Shall we Prune the search tree
    if (PruneTree)
    {
        if ((Side[depth]==WHITE) && (pieceValue>=BestScore[depth+1]))
            return  pieceValue;
        if ((Side[depth]==BLACK) && (pieceValue<=BestScore[depth+1]))
            return  pieceValue;
    }
    // Does the user wish to force the computer to finish the current move?
    if (forcePlay && (BestScore[depth]!=(Side[depth]==WHITE)?1:255))
        return  pieceValue;
	Chess_LedStat(status);
    // Initialise best node's score
    BestScore[depth] = (Side[depth]==WHITE)?1:255;
    // Find each piece and generate moves
    x = 4;
    for (i=0; i<8; i++)
    {
        x = (i & 1)?(x-i):(x+i);    // gen 43526170 seq, encourage center moves
        for (y=0; y<8; y++)
        {
            piece = BdLkup(x,y);
            if (piece!=NO_PIECE && (piece>>3)==Side[depth])
                MoveGen(depth,piece);
        }
    }
    return  BestScore[depth];
}
//---------------------------------------------------------------------------
static  void    DoChess(TChess* pChess)
{
    x2 = pChess->x2;
    y2 = pChess->y2;
    ok = FALSE;
    // Are we just checking a move?
    if (pChess->depth==0)
    {
        function = 0;
        CheckMove(pChess);
    }
    else
    {
        // Do computers move
        BestScore[pChess->depth+1] = (pChess->side==WHITE)?255:1;
        for (t2=pChess->depth; ; --t2)
        {
            Side[t2] = pChess->side;
            MoveOk[t2] = FALSE;
            // Initialise best node score
            BestScore[t2] = (pChess->side==WHITE)?1:255;
            if (!t2)
                break;
            pChess->side = (pChess->side ^ WHITE);
        }
        function = 2;
        pieceValue = 128;
        t1 = 0;
        kingF = FALSE;
        forcePlay = 0;
        MinMax(pChess->depth);
        pChess->x1 = BestX[pChess->depth];
        pChess->y1 = BestY[pChess->depth];
        pChess->x2 = BestP[pChess->depth];
        pChess->y2 = BestQ[pChess->depth];
    }
    pChess->ok = ok;
}
//---------------------------------------------------------------------------
static	void	CheckPromotePawn(Byte to,TSide side)
{
	// Promote PAWNs to QUEEN if other side reached
	if (PieceDefn(to)==PAWN)
	{
		if (to<8 && side==BLACK)
			SetPiece(to,B_QUEEN);
		if (to>55 && side==WHITE)
			SetPiece(to,W_QUEEN);	
	}
}
//---------------------------------------------------------------------------
static	void	CheckPerformCastle(Byte from,Byte to,TSide side)
{
	Byte	sq;

	// ONLY can castle one...adjusts the castle flag
	switch (PieceDefn(to))
	{
	case KING:
		castle &= (side==WHITE)?0x0F:0xF0;
		if (AbsDiff(from,to)==2)
		{
			// AutoMove ROOK into place
			if (to>from)
			{
				SetPiece((to-1),GetPiece(to+1));
				SetPiece((to+1),NO_PIECE);
				from = to+1;
				to--;
			}
			else
			{
				SetPiece((to+1),GetPiece(to-2));
				SetPiece((to-2),NO_PIECE);
				from = to-2;
				to++;
			}
			Chess_Beep();
			for (sq=Chess_ScanBoard(from,STATUS);sq!=from;)
			{
				Chess_Beep();
				Chess_Beep();
				sq = Chess_ScanBoard(from,STATUS);
			}
			for (sq=Chess_ScanBoard(to,STATUS);sq!=to;)
			{
				Chess_Beep();
				Chess_Beep();
				sq = Chess_ScanBoard(to,STATUS);
			}
		}
		break;
	case ROOK:
		if (Col(from)==0)
			castle &= (side==WHITE)?0xDF:0xFD;
		if (Col(from)==7)
			castle &= (side==WHITE)?0xBF:0xFB;
		break;
	}
}
//---------------------------------------------------------------------------
static	void 	ShowMoveLCD(Byte from,Byte to,Byte piece)
{
	static	const	char	PieceName[7][8] = 	{ 
													{"       "},
													{"Pawn   "},
													{"Knight "},
													{"Bishop "},
													{"Rook   "},
													{"Queen  "},
													{"King   "}
												};

	char	move[30];
	char*	pMove = move;
	char*	pChar;

	LCD_clear();
	pChar = ((piece>>3)==WHITE)?"Wht ":"Blk ";
	while (*pChar)
		*pMove++ = *pChar++;
	pChar = PieceName[piece&7];
	while (*pChar)
		*pMove++ = *pChar++;
	*pMove++ = ('A'+RANK(from));
	*pMove++ = ('1'+FILE(from));
	*pMove++ = ('A'+RANK(to));
	*pMove++ = ('1'+FILE(to));
	*pMove++ = 0;
	LCD_str(move);
}	
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------






//---------------------------------------------------------------------------





//---------------------------------------------------------------------------





//---------------------------------------------------------------------------






//---------------------------------------------------------------------------














