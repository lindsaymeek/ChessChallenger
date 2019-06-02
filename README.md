# ChessChallenger
An electronic chessboard utilising the Renesas H8/3687F as the central processing unit. 
by Lindsay Meek and Ilario Dimasi

The chessboard itself consists of a matrix of pressure sensors with LEDs along the rows and columns. 
The H8/3687F interfaces to the chessboard matrix, and scans it to detect the movement of pieces from a human opponent. 

A chess algorithm then devises counter moves, which are indicated using the row and column LEDs. 
The chess algorithm is capable of playing at four difficulty levels, corresponding to the number of moves ahead it is processing.
