#include <graphics.h>
#include <iostream>
#include <cstring>
#include <cstdio>
using namespace std;

// 窗口与棋盘尺寸
#define CHESS_W 640
#define CHESS_H 640
#define BLOCK 64
#define BOARD_OFFSET_X ((CHESS_W - BLOCK * 8) / 2)
#define BOARD_OFFSET_Y 40
const int circleR = 24;
// 对局终止标记
bool gameOver = false;
// 对局结果：0对局进行中 1白方胜利 2黑方胜利 3平局逼和
int gameResult = 0;
bool validGrid[8][8] = {false};
static int hoverR = -1, hoverC = -1;

// 最大记录50步，足够对局使用
#define MAX_STEP 50
// 存档结构：保存一步完整对局状态
struct GameState {
	int board[8][8];
	int turn;
	bool castlingAvail[6];
	int enPassantRow, enPassantCol;
};
GameState stepStack[MAX_STEP];
int stackTop = 0; // 栈顶，0=无存档

// 棋子统一大写
const char chessPieces[2][6] = {
	{'K','Q','R','B','N','P'},
	{'K','Q','R','B','N','P'}
};
// 棋盘 0空 1-6白棋 7-12黑棋
int board[8][8] = {
	{9,11,10,8,7,10,11,9},
	{12,12,12,12,12,12,12,12},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0,0},
	{6,6,6,6,6,6,6,6},
	{3,5,4,2,1,4,5,3}
};

int turn = 0;       // 0=白棋回合，1=黑棋回合
int selRow = -1;    // 选中棋子行，-1无选中
int selCol = -1;    // 选中棋子列
// 1.王车易位可用标记：顺序：白王、白左车、白右车、黑王、黑左车、黑右车
bool castlingAvail[6] = {true,true,true,true,true,true};
// 2.吃过路兵：记录上一回合走了两格的兵坐标，(-1,-1)代表无过路兵可吃
int enPassantRow = -1, enPassantCol = -1;
// 3.兵升变弹窗选择标记
bool needPromote = false;
int promoteTarRow, promoteTarCol;
int promoteSide = 0;

bool canMove(int sr, int sc, int tr, int tc);
bool isKingChecked(int kingRow, int kingCol, int attackerSide);
bool isAttacked(int r, int c, int attackerSide);

void findKingPos(int side, int &outR, int &outC) {
	int kingId = (side == 0) ? 1 : 7;
	for(int r = 0; r < 8; r++) {
		for(int c = 0; c < 8; c++) {
			if(board[r][c] == kingId) {
				outR = r;
				outC = c;
				return;
			}
		}
	}
}
// 重置所有棋局数据为开局初始状态
static void resetGame() {
	// 1. 恢复原始棋盘布局
	int initBoard[8][8] = {
		{9,11,10,8,7,10,11,9},
		{12,12,12,12,12,12,12,12},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{0,0,0,0,0,0,0,0},
		{6,6,6,6,6,6,6,6},
		{3,5,4,2,1,4,5,3}
	};
	for(int i=0; i<8; i++)
		for(int j=0; j<8; j++)
			board[i][j] = initBoard[i][j];

	// 2. 回合、选中清空
	turn = 0;
	selRow = -1;
	selCol = -1;
	memset(validGrid, 0, sizeof(validGrid));

	// 3. 王车易位全部恢复可用
	castlingAvail[0]=true;
	castlingAvail[1]=true;
	castlingAvail[2]=true;
	castlingAvail[3]=true;
	castlingAvail[4]=true;
	castlingAvail[5]=true;

	// 4. 过路兵清空
	enPassantRow = -1;
	enPassantCol = -1;

	// 5. 升变状态关闭
	needPromote = false;
	promoteSide = 0;
	promoteTarRow = 0;
	promoteTarCol = 0;

	// 6. 清空悔棋栈记录
	stackTop = 0;

	gameOver = false;
	gameResult = 0;
}
// 判断某个位置棋子属于哪一方：0白 1黑 -1空
int getPieceSide(int r, int c) {
	int p = board[r][c];
	if (p == 0) return -1;
	return p <= 6 ? 0 : 1;
}

// 判断是否可以从(sr,sc)走到(tr,tc) 简易基础走棋规则
bool canMove(int sr, int sc, int tr, int tc) {
	int piece = board[sr][sc];
	int target = board[tr][tc];
	int side = getPieceSide(sr, sc);
	int tSide = getPieceSide(tr, tc);

	// 禁止走到对方王，不能吃王
	if(target == 1 || target == 7)
		return false;
	// 不能走到己方棋子
	if (tSide == side)
		return false;

	int rDiff = tr - sr;
	int cDiff = tc - sc;
	int absR = abs(rDiff);
	int absC = abs(cDiff);

	bool pseudoLegal = false;

	// 王
	if (piece == 1 || piece == 7) {
		if(absR == 0 && absC == 2) {
			if(side == 0 && sr == 7 && sr == tr) {
				if(tc == 6 && castlingAvail[0] && castlingAvail[2]
				        && board[7][5]==0 && board[7][6]==0)
					pseudoLegal = true;
				if(tc == 2 && castlingAvail[0] && castlingAvail[1]
				        && board[7][3]==0 && board[7][2]==0 && board[7][1]==0)
					pseudoLegal = true;
			}
			if(side == 1 && sr == 0 && sr == tr) {
				if(tc == 6 && castlingAvail[3] && castlingAvail[5]
				        && board[0][5]==0 && board[0][6]==0)
					pseudoLegal = true;
				if(tc == 2 && castlingAvail[3] && castlingAvail[4]
				        && board[0][3]==0 && board[0][2]==0 && board[0][1]==0)
					pseudoLegal = true;
			}
		}
		if(absR <= 1 && absC <= 1)
			pseudoLegal = true;
	}
	// 后
	else if (piece == 2 || piece == 8) {
		if ((rDiff == 0 || cDiff == 0)) {
			int stepR = rDiff == 0 ? 0 : rDiff / absR;
			int stepC = cDiff == 0 ? 0 : cDiff / absC;
			int cr = sr + stepR, cc = sc + stepC;
			bool block = false;
			while (cr != tr || cc != tc) {
				if (board[cr][cc] != 0) {
					block = true;
					break;
				}
				cr += stepR;
				cc += stepC;
			}
			if(!block) pseudoLegal = true;
		}
		if (absR == absC) {
			int stepR = rDiff / absR;
			int stepC = cDiff / absC;
			int cr = sr + stepR, cc = sc + stepC;
			bool block = false;
			while (cr != tr || cc != tc) {
				if (board[cr][cc] != 0) {
					block = true;
					break;
				}
				cr += stepR;
				cc += stepC;
			}
			if(!block) pseudoLegal = true;
		}
	}
	// 车
	else if (piece == 3 || piece == 9) {
		if (rDiff != 0 && cDiff != 0) pseudoLegal = false;
		else {
			int stepR = rDiff == 0 ? 0 : rDiff / absR;
			int stepC = cDiff == 0 ? 0 : cDiff / absC;
			int cr = sr + stepR, cc = sc + stepC;
			bool block = false;
			while (cr != tr || cc != tc) {
				if (board[cr][cc] != 0) {
					block = true;
					break;
				}
				cr += stepR;
				cc += stepC;
			}
			if(!block) pseudoLegal = true;
		}
	}
	// 象
	else if (piece == 4 || piece == 10) {
		if (absR != absC) pseudoLegal = false;
		else {
			int stepR = rDiff / absR;
			int stepC = cDiff / absC;
			int cr = sr + stepR, cc = sc + stepC;
			bool block = false;
			while (cr != tr || cc != tc) {
				if (board[cr][cc] != 0) {
					block = true;
					break;
				}
				cr += stepR;
				cc += stepC;
			}
			if(!block) pseudoLegal = true;
		}
	}
	// 马
	else if (piece == 5 || piece == 11) {
		if ((absR == 2 && absC == 1) || (absR == 1 && absC == 2))
			pseudoLegal = true;
	}
	// 白兵
	else if (piece == 6) {
		if(absC == 1 && rDiff == -1 && tr == enPassantRow && tc == enPassantCol)
			pseudoLegal = true;
		if (cDiff == 0 && rDiff == -1 && target == 0)
			pseudoLegal = true;
		if (sr == 6 && cDiff == 0 && rDiff == -2 && board[sr-1][sc] == 0 && target == 0)
			pseudoLegal = true;
		if (absC == 1 && rDiff == -1 && tSide == 1)
			pseudoLegal = true;
	}
	// 黑兵
	else if (piece == 12) {
		if(absC == 1 && rDiff == 1 && tr == enPassantRow && tc == enPassantCol)
			pseudoLegal = true;
		if (cDiff == 0 && rDiff == 1 && target == 0)
			pseudoLegal = true;
		if (sr == 1 && cDiff == 0 && rDiff == 2 && board[sr+1][sc] == 0 && target == 0)
			pseudoLegal = true;
		if (absC == 1 && rDiff == 1 && tSide == 0)
			pseudoLegal = true;
	}

	// 基础走法都不合法，直接返回
	if(!pseudoLegal)
		return false;

	/// 模拟走子，校验走完己方王是否被将军
	int moveSide = side;
	int kr, kc;
	findKingPos(moveSide, kr, kc);

	int bakSrc = board[sr][sc];
	int bakTar = board[tr][tc];
	board[tr][tc] = board[sr][sc];
	board[sr][sc] = 0;

// 根据【原始棋子类型】判断是否为王，刷新国王坐标
	if(bakSrc == 1 || bakSrc == 7) {
		kr = tr;
		kc = tc;
	}
	bool kingAttacked = isAttacked(kr, kc, 1 - moveSide);

// 还原棋盘
	board[sr][sc] = bakSrc;
	board[tr][tc] = bakTar;

	if(kingAttacked)
		return false;

	return true;
}

void drawBoard() {
	for(int row = 0; row < 8; row++) {
		for(int col = 0; col < 8; col++) {
			int gridX = BOARD_OFFSET_X + col * BLOCK;
			int gridY = BOARD_OFFSET_Y + row * BLOCK;
			if((row + col) % 2 == 0)
				setfillcolor(RGB(240,210,180));
			else
				setfillcolor(RGB(120,80,50));
			solidrectangle(gridX, gridY, gridX + BLOCK, gridY + BLOCK);
		}
	}

	// 绘制左侧数字 1~8（对应row 0=8，row7=1）
	settextstyle(20, 0, "Microsoft YaHei");
	settextcolor(WHITE);
	setbkmode(TRANSPARENT);
	for(int row = 0; row < 8; row++) {
		int num = 8 - row;
		int y = BOARD_OFFSET_Y + row * BLOCK + BLOCK / 2 - 10;
		int x = BOARD_OFFSET_X - 25;
		char buf[2];
		sprintf(buf, "%d", num);
		outtextxy(x, y, buf);
	}

	// 绘制下方字母 a~h（col0=a, col7=h）
	char letterTable[8] = {'a','b','c','d','e','f','g','h'};
	for(int col = 0; col < 8; col++) {
		int x = BOARD_OFFSET_X + col * BLOCK + BLOCK / 2 - 8;
		int y = BOARD_OFFSET_Y + BLOCK * 8 + 8;
		char singleChar[2] = {letterTable[col], '\0'};
		outtextxy(x, y, singleChar);
	}

	// 全部合法格子浅蓝底色（可选，不加也能只用悬停黄圈）
	setfillcolor(RGB(100, 180, 255));
	for(int r=0; r<8; r++) {
		for(int c=0; c<8; c++) {
			if(validGrid[r][c]) {
				int x = BOARD_OFFSET_X + c * BLOCK;
				int y = BOARD_OFFSET_Y + r * BLOCK;
				solidrectangle(x+4, y+4, x+BLOCK-4, y+BLOCK-4);
			}
		}
	}
}

void saveState() {
	if (stackTop >= MAX_STEP) return;
	// 复制棋盘
	for(int i=0; i<8; i++)
		for(int j=0; j<8; j++)
			stepStack[stackTop].board[i][j] = board[i][j];
	// 复制回合
	stepStack[stackTop].turn = turn;
	// 复制易位权限
	for(int i=0; i<6; i++)
		stepStack[stackTop].castlingAvail[i] = castlingAvail[i];
	// 复制过路兵
	stepStack[stackTop].enPassantRow = enPassantRow;
	stepStack[stackTop].enPassantCol = enPassantCol;

	stackTop++;
}
void undoStep() {
	if (stackTop <= 0) return; // 没有步骤可悔
	stackTop--;
	gameOver = false;
	gameResult = 0;
	GameState& s = stepStack[stackTop];
	// 恢复棋盘
	for(int i=0; i<8; i++)
		for(int j=0; j<8; j++)
			board[i][j] = s.board[i][j];
	// 恢复回合
	turn = s.turn;
	// 恢复易位权限
	for(int i=0; i<6; i++)
		castlingAvail[i] = s.castlingAvail[i];
	// 恢复过路兵
	enPassantRow = s.enPassantRow;
	enPassantCol = s.enPassantCol;
	// 清空选中
	selRow = -1;
	selCol = -1;
	memset(validGrid, 0, sizeof(validGrid));
}
// 仅判断坐标(r,c)是否被attackerSide的棋子攻击（纯路径，不校验送王）
bool isAttacked(int r, int c, int attackerSide) {
	// 八方向：王、后、车、象
	int dirs[8][2] = {{-1,0},{1,0},{0,-1},{0,1},{-1,-1},{-1,1},{1,-1},{1,1}};
	// 马8个点位
	int knightDir[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};

	// 1. 马检测
	for(int d=0; d<8; d++) {
		int nr = r + knightDir[d][0];
		int nc = c + knightDir[d][1];
		if(nr<0||nr>=8||nc<0||nc>=8) continue;
		int p = board[nr][nc];
		if(getPieceSide(nr, nc) == attackerSide && (p==5||p==11))
			return true;
	}

	// 2. 八方向直线：车/象/后/王
	for(int d=0; d<8; d++) {
		int dr = dirs[d][0];
		int dc = dirs[d][1];
		int nr = r + dr;
		int nc = c + dc;
		while(nr>=0&&nr<8&&nc>=0&&nc<8) {
			int p = board[nr][nc];
			if(p == 0) {
				nr += dr;
				nc += dc;
				continue;
			}
			int pieceSide = getPieceSide(nr, nc);
			//不是攻击方的棋子，障碍物阻断射线直接跳出
			if(pieceSide != attackerSide) break;
			// 王 一步攻击范围
			if((p==1||p==7) && abs(nr-r)==1 && abs(nc-c)==1)
				return true;
			// 车：横向纵向方向
			if((p==3||p==9) && (dr==0||dc==0))
				return true;
			// 象：斜线方向
			if((p==4||p==10) && dr!=0 && dc!=0)
				return true;
			// 后任意方向
			if(p==2||p==8)
				return true;
			//命中同阵营非对应兵种，停止扫描
			break;
		}
	}

	// 3. 兵斜向前攻击
	if(attackerSide == 0) { // 白方攻击：白兵(6)斜向上吃，目标格子的斜下方有白兵
		if(r + 1 < 8) {
			if(c - 1 >= 0 && board[r+1][c-1]==6) return true;
			if(c + 1 < 8 && board[r+1][c+1]==6) return true;
		}
	} else { // 黑方攻击：黑兵(12)斜向下吃，目标格子的斜上方有黑兵
		if(r - 1 >= 0) {
			if(c - 1 >= 0 && board[r-1][c-1]==12) return true;
			if(c + 1 < 8 && board[r-1][c+1]==12) return true;
		}
	}
	return false;
}

// 现在isKingChecked直接调用无递归的isAttacked
bool isKingChecked(int kingRow, int kingCol, int attackerSide) {
	return isAttacked(kingRow, kingCol, attackerSide);
}
bool hasAnyValidMove(int curSide) {
	// canMove已经内置送王/将军校验，直接判断有没有能走的格子即可
	for(int sr = 0; sr < 8; sr++) {
		for(int sc = 0; sc < 8; sc++) {
			if(getPieceSide(sr, sc) != curSide) continue;
			for(int tr = 0; tr < 8; tr++) {
				for(int tc = 0; tc < 8; tc++) {
					if(canMove(sr, sc, tr, tc))
						return true;
				}
			}
		}
	}
	return false;
}
// 判断当前走棋方是否处于将军状态
bool CurrentTurnInCheck() {
	int kr, kc;
	findKingPos(turn, kr, kc);
	// attackerSide = 对手 1-turn
	return isKingChecked(kr, kc, 1 - turn);
}
void drawAllPieces() {
	for(int row = 0; row < 8; row++) {
		for(int col = 0; col < 8; col++) {
			int pieceCode = board[row][col];
			if(pieceCode == 0)
				continue;

			int colorIdx, typeIdx;
			if(pieceCode <= 6) {
				colorIdx = 0;
				typeIdx = pieceCode - 1;
			} else {
				colorIdx = 1;
				typeIdx = pieceCode - 7;
			}

			int centerX = BOARD_OFFSET_X + col * BLOCK + BLOCK / 2;
			int centerY = BOARD_OFFSET_Y + row * BLOCK + BLOCK / 2;

			// 选中棋子黄色外圈
			if (row == selRow && col == selCol) {
				setlinecolor(YELLOW);
				setlinestyle(PS_SOLID, 3);
				circle(centerX, centerY, circleR + 4);
				setlinecolor(BLACK);
				setlinestyle(PS_SOLID, 1);
			}

			// 被将军的王红色外圈
			if((pieceCode == 1 || pieceCode == 7)) {
				int side = getPieceSide(row, col);
				int kr, kc;
				findKingPos(side, kr, kc);
				if(kr == row && kc == col && isKingChecked(row, col, 1-side)) {
					setlinecolor(RED);
					setlinestyle(PS_SOLID, 3);
					circle(centerX, centerY, circleR + 8);
					setlinecolor(BLACK);
					setlinestyle(PS_SOLID, 1);
				}
			}

			// 棋子底色圆
			if(colorIdx == 0)
				setfillcolor(WHITE);
			else
				setfillcolor(BLACK);
			solidcircle(centerX, centerY, circleR);

			// 大写字母
			settextstyle(36, 0, "Microsoft YaHei");
			setbkmode(TRANSPARENT);
			if(colorIdx == 0)
				settextcolor(BLACK);
			else
				settextcolor(WHITE);
			char buf[2] = {chessPieces[colorIdx][typeIdx], '\0'};
			int textX = centerX - 9;
			int textY = centerY - 18;
			outtextxy(textX, textY, buf);
		}
	}

	// ============ 鼠标悬停在合法格子，绘制黄色空心圆圈 ============
	if(selRow != -1 && hoverR != -1 && hoverC != -1) {
		if(validGrid[hoverR][hoverC]) {
			int cx = BOARD_OFFSET_X + hoverC * BLOCK + BLOCK / 2;
			int cy = BOARD_OFFSET_Y + hoverR * BLOCK + BLOCK / 2;
			setlinecolor(YELLOW);
			setlinestyle(PS_SOLID, 4);
			circle(cx, cy, circleR + 6);
			setlinecolor(BLACK);
			setlinestyle(PS_SOLID, 1);
		}
	}
}
// 绘制返回按钮 + 回合样式改造
void drawBackBtn() {
	int btnX = 460;
	int btnY = 590;
	setfillcolor(RED);
	solidrectangle(btnX, btnY, btnX + 110, btnY + 45);
	settextcolor(WHITE);
	settextstyle(22, 0, "Microsoft YaHei");
	setbkmode(TRANSPARENT);
	outtextxy(btnX + 18, btnY + 12, "返回主菜单");
	// 新增悔棋按钮
	int undoX = 320;
	int undoY = 590;
	setfillcolor(RGB(242,198,48));
	settextcolor(BLACK);
	solidrectangle(undoX, undoY, undoX + 110, undoY + 45);
	outtextxy(undoX + 28, undoY + 12, "悔棋");
	// 顶部回合提示
	int tipX = BOARD_OFFSET_X;
	int tipY = 6;
	int tipW = 170;
	int tipH = 32;
	setfillcolor(RGB(128,128,128));
	solidrectangle(tipX, tipY, tipX + tipW, tipY + tipH);
	settextstyle(26,0,"Microsoft YaHei");
	setbkmode(TRANSPARENT);
	if (turn == 0) {
		settextcolor(WHITE);
		outtextxy(tipX+8, tipY+4, "当前回合: 白棋");
	} else {
		settextcolor(BLACK);
		outtextxy(tipX+8, tipY+4, "当前回合: 黑棋");
	}
	if(CurrentTurnInCheck()) {
		settextcolor(RED);
		outtextxy(tipX + tipW + 10, tipY + 4, "【将军！】");
	}
}
void runChess() {
	resetGame();
	initgraph(CHESS_W, CHESS_H);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "国际象棋");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/chess.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	MOUSEMSG mouseMsg;
	while(true) {
		BeginBatchDraw();
		cleardevice();
		drawBoard();
		drawAllPieces();
		drawBackBtn();

		// 兵升变弹窗
		if (needPromote) {
			setfillcolor(RGB(220,220,220));
			solidrectangle(120,220,520,340);
			const char promChar[4]= {'Q','R','B','N'};
			const int promCode[4]= {2,3,4,5};
			for(int i=0; i<4; i++) {
				int bx = 140 + i * 90;
				int by = 250;
				solidrectangle(bx, by, bx + 70, by + 50);
				char txtBuf[2] = {promChar[i], '\0'};
				outtextxy(bx + 25, by + 12, txtBuf);
			}
		}

		// 对局结束结算弹窗
		if(gameOver) {
			setfillcolor(RGB(230,230,230));
			solidrectangle(150,200,490,360);
			settextstyle(30, 0, "Microsoft YaHei");
			setbkmode(TRANSPARENT);
			if(gameResult == 1) outtextxy(190, 240, "白方胜利!");
			else if(gameResult == 2) outtextxy(190, 240, "黑方胜利!");
			else if(gameResult == 3) outtextxy(200, 240, "逼和平局!");
			settextstyle(20, 0, "Microsoft YaHei");
			outtextxy(180, 290, "点击悔棋撤销对局结果");
			outtextxy(180, 315, "其余位置点击关闭本局");
		}
		EndBatchDraw();

		mouseMsg = GetMouseMsg();
		if(mouseMsg.uMsg == WM_MOUSEMOVE) {
			int mx = mouseMsg.x - BOARD_OFFSET_X;
			int my = mouseMsg.y - BOARD_OFFSET_Y;
			hoverC = mx / BLOCK;
			hoverR = my / BLOCK;
			if(hoverR<0||hoverR>=8||hoverC<0||hoverC>=8) {
				hoverR=-1;
				hoverC=-1;
			}
			continue;
		}
		if(mouseMsg.uMsg == WM_LBUTTONDOWN) {
			// =========结算状态特殊交互：允许悔棋、其余点位关闭窗口=========
			if(gameOver) {
				int undoX = 320;
				int undoY = 570;
				// 命中悔棋按钮：回退棋局、解除结算锁定
				if(mouseMsg.x >= undoX && mouseMsg.x <= undoX+110
				        && mouseMsg.y >= undoY && mouseMsg.y <= undoY+45) {
					undoStep();
					gameOver = false;
					gameResult = 0;
					continue;
				}
				// 点击别的区域直接退出本局
				closegraph();
				return;
			}

			// 返回主菜单按钮
			if(mouseMsg.x >= 460 && mouseMsg.x <= 460+110
			        && mouseMsg.y >= 590 && mouseMsg.y <= 590+45) {
				closegraph();
				return;
			}
			// 普通对局状态：悔棋按钮逻辑
			int undoX = 320;
			int undoY = 570;
			if(mouseMsg.x >= undoX && mouseMsg.x <= undoX+110
			        && mouseMsg.y >= undoY && mouseMsg.y <= undoY+45) {
				undoStep();
				continue;
			}
			// 升变弹窗点击判定
			if (needPromote) {
				int mx = mouseMsg.x;
				int my = mouseMsg.y;
				const int promCode[4]= {2,3,4,5};
				for(int i=0; i<4; i++) {
					int bx = 140 + i * 90;
					int by = 250;
					if(mx >= bx && mx <= bx+70 && my >= by && my <= by+50) {
						int base = (promoteSide == 0) ? 0 : 6;
						board[promoteTarRow][promoteTarCol] = promCode[i] + base;
						needPromote = false;
						turn = 1 - turn;
						int kingR, kingC;
						findKingPos(turn, kingR, kingC);
						bool inCheck = isKingChecked(kingR, kingC, 1 - turn);
						bool canMoveFlag = hasAnyValidMove(turn);
						if(!canMoveFlag) {
							gameOver = true;
							if(inCheck) gameResult = (turn==0) ? 2 : 1;
							else gameResult = 3;
						}
						break;
					}
				}
				continue;
			}

			// 鼠标坐标换算棋盘行列
			int mx = mouseMsg.x - BOARD_OFFSET_X;
			int my = mouseMsg.y - BOARD_OFFSET_Y;
			int r = my / BLOCK;
			int c = mx / BLOCK;
			if (r < 0 || r >= 8 || c < 0 || c >= 8)
				continue;
			int clickSide = getPieceSide(r,c);

			if (selRow == -1) {
				if (clickSide == turn) {
					bool hasValid = false;
					for(int tr=0; tr<8 && !hasValid; tr++) {
						for(int tc=0; tc<8 && !hasValid; tc++) {
							if(canMove(r,c,tr,tc)) {
								hasValid = true;
							}
						}
					}
					if(hasValid) {
						selRow = r;
						selCol = c;
						// 清空旧标记
						memset(validGrid, 0, sizeof(validGrid));
						// 回填全部合法落点
						for(int tr=0; tr<8; tr++)
							for(int tc=0; tc<8; tc++)
								if(canMove(r,c,tr,tc))
									validGrid[tr][tc] = true;
					}
				}
			} else {
				// 点同一格取消选中
				if (r == selRow && c == selCol) {
					selRow = -1;
					selCol = -1;
					memset(validGrid, 0, sizeof(validGrid));
				}
				// 切换选中己方其他棋子
				else if (clickSide == turn) {
					bool hasValid = false;
					for(int tr=0; tr<8 && !hasValid; tr++) {
						for(int tc=0; tc<8 && !hasValid; tc++) {
							if(canMove(r,c,tr,tc)) {
								hasValid = true;
							}
						}
					}
					if(hasValid) {
						selRow = r;
						selCol = c;
						// 清空旧标记
						memset(validGrid, 0, sizeof(validGrid));
						// 回填全部合法落点
						for(int tr=0; tr<8; tr++)
							for(int tc=0; tc<8; tc++)
								if(canMove(r,c,tr,tc))
									validGrid[tr][tc] = true;
					}
				}
				// 执行走子逻辑
				else {
					if (canMove(selRow, selCol, r, c)) {
						saveState();
						int movedPiece = board[selRow][selCol];
						// 王车易位挪动车
						if((movedPiece == 1 || movedPiece == 7) && abs(c - selCol) == 2) {
							int side = getPieceSide(selRow, selCol);
							if (side == 0) {
								if(c == 6) {
									board[7][5] = board[7][7];
									board[7][7] = 0;
								}
								if(c == 2) {
									board[7][3] = board[7][0];
									board[7][0] = 0;
								}
							} else if (side == 1) {
								if(c == 6) {
									board[0][5] = board[0][7];
									board[0][7] = 0;
								}
								if(c == 2) {
									board[0][3] = board[0][0];
									board[0][0] = 0;
								}
							}
						}
						// 吃过路兵删除被越过的兵
						if((movedPiece == 6 || movedPiece == 12) && c == enPassantCol && r == enPassantRow) {
							board[selRow][c] = 0;
						}
						// 棋盘棋子位移
						board[r][c] = board[selRow][selCol];
						board[selRow][selCol] = 0;
						// 锁定后续禁止易位
						if(movedPiece == 1) castlingAvail[0] = false;
						if(movedPiece == 7) castlingAvail[3] = false;
						if(selRow == 7 && selCol == 0) castlingAvail[1] = false;
						if(selRow == 7 && selCol == 7) castlingAvail[2] = false;
						if(selRow == 0 && selCol == 0) castlingAvail[4] = false;
						if(selRow == 0 && selCol == 7) castlingAvail[5] = false;
						// 更新过路兵坐标
						enPassantRow = -1;
						enPassantCol = -1;
						if((movedPiece == 6 && selRow - r == 2) || (movedPiece == 12 && r - selRow == 2)) {
							enPassantRow = (selRow + r) / 2;
							enPassantCol = c;
						}
						// 判断是否触发兵升变
						if((movedPiece == 6 && r == 0) || (movedPiece == 12 && r == 7)) {
							needPromote = true;
							promoteTarRow = r;
							promoteTarCol = c;
							promoteSide = turn;
						} else {
							turn = 1 - turn;
							int kingR, kingC;
							findKingPos(turn, kingR, kingC);
							bool inCheck = isKingChecked(kingR, kingC, 1 - turn);
							bool canMoveFlag = hasAnyValidMove(turn);
							if(!canMoveFlag) {
								gameOver = true;
								if(inCheck) gameResult = (turn == 0) ? 2 : 1;
								else gameResult = 3;
							}
						}
						selRow = -1;
						selCol = -1;
						memset(validGrid, 0, sizeof(validGrid));
					}
				}
			}
		}
	}
	closegraph();
}
