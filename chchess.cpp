#include <graphics.h>
#include <iostream>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <stack>
using namespace std;

// 棋盘常量：象棋纵向10格、横向9格
#define XQ_ROW 10
#define XQ_COL 9
#define XQ_GRID 45
#define XQ_OFFSET 35
#define CHESS_R 18

// 底部按钮参数
#define BTN_Y  XQ_OFFSET + XQ_ROW * XQ_GRID + 12
#define BTN_H 36
#define BTN_X1 160
#define BTN_X2 330
#define BTN_UNDO_X1 70
#define BTN_UNDO_X2 220
#define BTN_BACK_X1 250
#define BTN_BACK_X2 420

// 九宫横向列范围 3~5（下标）
#define PALACE_LEFT 3
#define PALACE_RIGHT 5
// 黑方九宫纵向：0~2；红方九宫纵向：7~9

// 单步存档结构体：存储棋盘+回合
struct ChessSnapshot {
	int boardData[XQ_ROW][XQ_COL];
	int turnVal;
};
static stack<ChessSnapshot> undoStack; // 悔棋存档栈

// 棋子类型：0空，红1~7=帅车马炮相兵；黑8~14=将马士炮象卒
enum ChessType {
    EMPTY,
    R_KING,R_HORSE,R_CAR,R_CANNON,R_ELE,R_OFFICER,R_SOL,
    B_KING,B_HORSE,B_CAR,B_CANNON,B_ELE,B_OFFICER,B_SOL
};
// 棋子汉字映射
const char* chessName[] = {
	"",
	"帅","馬","車","炮","相","仕","兵",
	"将","馬","車","砲","象","士","卒"
};
static int board[XQ_ROW][XQ_COL] = {0};
static int selectR = -1, selectC = -1;
static bool backMenu = false;
static int turn = 1; //1红方回合，2黑方回合
static bool gameOver = false;
static int winner = 0; //0无，1红胜，2黑胜
static int hoverR = -1, hoverC = -1; // 鼠标悬浮的棋盘行列坐标

static void initXiangqiBoard();
static void resetXqGame();
static void drawBg();
static void drawBoard();
static void drawAllChess();
static void drawBtn();
static bool hitBackBtn(int mx,int my);
static bool pos2Grid(int mx,int my,int &r,int &c);
static void findKing(int &rr,int &rc,int &br,int &bc);
static bool posBeAttacked(int simBoard[XQ_ROW][XQ_COL], int tarR, int tarC, int attackerSide);
static bool sideInCheck(int simBoard[XQ_ROW][XQ_COL], int checkSide);
static bool playerHasValidMoves(int side);
static void drawPreviewChess();
static int checkButtonClick(int mx, int my);

// 初始化标准开局排布
void initXiangqiBoard() {
	// ========== 上方：黑方（第0行=底线，第2行炮，第3行卒）==========
	// 下标0~8：车、马、象、士、将、士、象、马、车
	board[0][0] = B_CAR;
	board[0][1] = B_HORSE;
	board[0][2] = B_ELE;
	board[0][3] = B_OFFICER;
	board[0][4] = B_KING;
	board[0][5] = B_OFFICER;
	board[0][6] = B_ELE;
	board[0][7] = B_HORSE;
	board[0][8] = B_CAR;

	// 黑方双炮：第2行第1列、第7列
	board[2][1] = B_CANNON;
	board[2][7] = B_CANNON;

	// 黑方5枚卒：第3行 0/2/4/6/8
	board[3][0] = B_SOL;
	board[3][2] = B_SOL;
	board[3][4] = B_SOL;
	board[3][6] = B_SOL;
	board[3][8] = B_SOL;

	// ========== 下方：红方（第9行底线、第7行炮、第6行兵）==========
	board[9][0] = R_CAR;
	board[9][1] = R_HORSE;
	board[9][2] = R_ELE;
	board[9][3] = R_OFFICER;
	board[9][4] = R_KING;
	board[9][5] = R_OFFICER;
	board[9][6] = R_ELE;
	board[9][7] = R_HORSE;
	board[9][8] = R_CAR;

	board[7][1] = R_CANNON;
	board[7][7] = R_CANNON;

	board[6][0] = R_SOL;
	board[6][2] = R_SOL;
	board[6][4] = R_SOL;
	board[6][6] = R_SOL;
	board[6][8] = R_SOL;
}

// 渲染全部棋子：圆形底色+圈内汉字
void drawAllChess() {
	for(int r=0; r<XQ_ROW; r++) {
		for(int c=0; c<XQ_COL; c++) {
			int type = board[r][c];
			if(type==EMPTY) continue;
			int px = XQ_OFFSET + c*XQ_GRID;
			int py = XQ_OFFSET + r*XQ_GRID;
			// 选中棋子黄色高亮外圈
			if(r==selectR && c==selectC) setlinecolor(YELLOW);
			else setlinecolor(BLACK);
			// 区分红黑底色
			if(type <= R_SOL) {
				setfillcolor(RGB(255,242,242));//红棋浅红底
				settextcolor(RED);
			} else {
				setfillcolor(RGB(242,242,242));//黑棋浅灰底
				settextcolor(BLACK);
			}
			solidcircle(px,py,CHESS_R);
			circle(px,py,CHESS_R);
			// 绘制棋子汉字
			settextstyle(18,0,"宋体");
			setbkmode(TRANSPARENT);
			outtextxy(px-9,py-11,chessName[type]);
		}
	}
}
static void resetXqGame() {
	fill(&board[0][0],&board[XQ_ROW][0],EMPTY);
	selectR=selectC=-1;
	backMenu=false;
	turn = 1;
	gameOver = false;
	winner = 0;
	while(!undoStack.empty()) undoStack.pop(); // 清空历史存档
	initXiangqiBoard();
}
// 绘制背景
void drawBg() {
	setfillcolor(RGB(182,182,182));
	solidrectangle(0,0,getwidth(),getheight());
}
static void copyBoard(int dst[XQ_ROW][XQ_COL],int src[XQ_ROW][XQ_COL]) {
	for(int r=0; r<XQ_ROW; r++)
		for(int c=0; c<XQ_COL; c++)
			dst[r][c]=src[r][c];
}
// 画象棋棋盘网格、九宫斜线、楚河汉界
void drawBoard() {
	setlinecolor(BLACK);
	setlinestyle(PS_SOLID, 2);
	// 纵向竖线
	for(int c=0; c<XQ_COL; c++) {
		int x = XQ_OFFSET + c*XQ_GRID;
		line(x,XQ_OFFSET,x,XQ_OFFSET+4*XQ_GRID);
		line(x,XQ_OFFSET+5*XQ_GRID,x,XQ_OFFSET+9*XQ_GRID);
	}
	// 横向横线
	for(int r=0; r<XQ_ROW; r++) {
		int y = XQ_OFFSET + r*XQ_GRID;
		line(XQ_OFFSET,y,XQ_OFFSET+8*XQ_GRID,y);
	}
	// 九宫斜线（上方黑方）
	int topX = XQ_OFFSET+3*XQ_GRID,topY=XQ_OFFSET;
	line(topX,topY,topX+2*XQ_GRID,topY+2*XQ_GRID);
	line(topX+2*XQ_GRID,topY,topX,topY+2*XQ_GRID);
	// 下方红方九宫斜线
	int botX=XQ_OFFSET+3*XQ_GRID,botY=XQ_OFFSET+7*XQ_GRID;
	line(botX,botY,botX+2*XQ_GRID,botY+2*XQ_GRID);
	line(botX+2*XQ_GRID,botY,botX,botY+2*XQ_GRID);
	// 楚河汉界文字
	settextcolor(BLACK);
	settextstyle(22,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(XQ_OFFSET+XQ_GRID*2, XQ_OFFSET+4.35*XQ_GRID,"楚河        汉界");
}

// 绘制底部返回按钮
static void drawBtn() {
	// 悔棋按钮（黄色）
	setfillcolor(RGB(242, 198, 48));
	solidrectangle(BTN_UNDO_X1, BTN_Y, BTN_UNDO_X2, BTN_Y+BTN_H);
	settextcolor(BLACK);
	settextstyle(16,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN_UNDO_X1+45,BTN_Y+8,"悔棋");

	// 返回主菜单按钮（原有红色）
	setfillcolor(RGB(232,70,70));
	solidrectangle(BTN_BACK_X1,BTN_Y,BTN_BACK_X2,BTN_Y+BTN_H);
	settextcolor(WHITE);
	settextstyle(16,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(BTN_BACK_X1+22,BTN_Y+8,"返回主菜单");
}
// 补充悔棋按钮点击判定
static bool hitUndoBtn(int mx,int my) {
	if(my<BTN_Y || my>BTN_Y+BTN_H) return false;
	return mx >= BTN_UNDO_X1 && mx <= BTN_UNDO_X2;
}
// 更新旧返回按钮判定坐标
static bool hitBackBtn(int mx,int my) {
	if(my<BTN_Y || my>BTN_Y+BTN_H) return false;
	return mx>=BTN_BACK_X1 && mx<=BTN_BACK_X2;
}
static void pushSnapshot() {
	ChessSnapshot snap;
	copyBoard(snap.boardData, board);
	snap.turnVal = turn;
	undoStack.push(snap);
}

// 屏幕坐标转棋盘行列
bool pos2Grid(int mx,int my,int &r,int &c) {
	r=(my-XQ_OFFSET+XQ_GRID/2)/XQ_GRID;
	c=(mx-XQ_OFFSET+XQ_GRID/2)/XQ_GRID;
	return r>=0&&r<XQ_ROW&&c>=0&&c<XQ_COL;
}
// 判断坐标是否处在黑方九宫
static bool inBlackPalace(int r,int c) {
	return r >= 0 && r <= 2 && c >= PALACE_LEFT && c <= PALACE_RIGHT;
}
// 判断坐标是否处在红方九宫
static bool inRedPalace(int r,int c) {
	return r >= 7 && r <= 9 && c >= PALACE_LEFT && c <= PALACE_RIGHT;
}
// 校验两点竖直线中间有没有棋子（将帅对视检测）
static bool kingFaceEachOther(int srcR,int srcC,int dstR,int dstC) {
	if(srcC != dstC) return false;
	int minR = min(srcR,dstR), maxR = max(srcR,dstR);
	for(int r = minR + 1; r < maxR; r++) {
		if(board[r][srcC] != EMPTY) return false;
	}
	return true;
}
// 直线路径障碍物统计：返回路径棋子数量（车、炮复用）
static int countBlockOnLine(int sr,int sc,int tr,int tc) {
	int blockCnt = 0;
	if(sr == tr) { //水平横向
		int st = min(sc,tc), ed = max(sc,tc);
		for(int c=st+1; c<ed; c++) if(board[sr][c]!=EMPTY) blockCnt++;
	} else if(sc == tc) { //竖直纵向
		int st = min(sr,tr), ed = max(sr,tr);
		for(int r=st+1; r<ed; r++) if(board[r][sc]!=EMPTY) blockCnt++;
	}
	return blockCnt;
}
// 马蹩腿校验
static bool isHorseBlock(int sr,int sc,int dr,int dc) {
	int drR = dr - sr, drC = dc - sc;
	if(drR == -2) return board[sr-1][sc] != EMPTY;
	if(drR == 2) return board[sr+1][sc] != EMPTY;
	if(drC == -2) return board[sr][sc-1] != EMPTY;
	if(drC == 2) return board[sr][sc+1] != EMPTY;
	return false;
}
// 象眼蹩腿校验
static bool isEleBlock(int sr,int sc,int dr,int dc) {
	int midR = (sr+dr)/2, midC = (sc+dc)/2;
	return board[midR][midC] != EMPTY;
}
static bool canMove(int sr,int sc,int tr,int tc) {
	if(sr < 0 || sr >= XQ_ROW || sc < 0 || sc >= XQ_COL) return false;
	if(tr < 0 || tr >= XQ_ROW || tc < 0 || tc >= XQ_COL) return false;
	int self = board[sr][sc];
	int target = board[tr][tc];
	// 禁止移动到己方棋子位置
	if(target != EMPTY && ((self <= R_SOL && target <= R_SOL) || (self >= B_KING && target >= B_KING)))
		return false;
	int dR = abs(tr - sr), dC = abs(tc - sc);
	switch(self) {
			//====红方棋子校验====
		case R_KING:
			if(!inRedPalace(tr,tc)) return false;
			if(!((dR==1&&dC==0)||(dR==0&&dC==1))) return false;
			// 防止将帅对视
			if(target == B_KING && kingFaceEachOther(sr,sc,tr,tc)) return false;
			return true;
		case R_OFFICER:
			if(!inRedPalace(tr,tc)) return false;
			return dR == 1 && dC == 1;
		case R_ELE:
			// 象不能过河（红象行棋r≥5=跨过楚河）
			if(tr <= 4) return false;
			if(!(dR == 2 && dC == 2)) return false;
			return !isEleBlock(sr,sc,tr,tc);
		case R_HORSE:
			if(!((dR==2&&dC==1)||(dR==1&&dC==2))) return false;
			return !isHorseBlock(sr,sc,tr,tc);
		case R_CAR:
			if(dR != 0 && dC != 0) return false;
			return countBlockOnLine(sr,sc,tr,tc) == 0;
		case R_CANNON: {
			if(dR != 0 && dC != 0) return false;
			int blocks = countBlockOnLine(sr,sc,tr,tc);
			//空位路径零遮挡；吃子必须恰好1个炮架
			if(target == EMPTY) return blocks == 0;
			else return blocks == 1;
		}
		case R_SOL: {
			int offsetR = tr - sr;
			//未过河（r>4）仅能向上（行号变小）走一格
			if(sr >= 5) {
				if(offsetR != -1 || dC != 0) return false;
			} else { //过河：上、左、右单格
				bool valid = (offsetR == -1 && dC==0) || (offsetR==0 && dC==1);
				if(!valid) return false;
			}
			return true;
		}
		//====黑方棋子校验====
		case B_KING:
			if(!inBlackPalace(tr,tc)) return false;
			if(!((dR==1&&dC==0)||(dR==0&&dC==1))) return false;
			if(target == R_KING && kingFaceEachOther(sr,sc,tr,tc)) return false;
			return true;
		case B_OFFICER:
			if(!inBlackPalace(tr,tc)) return false;
			return dR == 1 && dC == 1;
		case B_ELE:
			if(tr >= 5) return false;
			if(!(dR == 2 && dC == 2)) return false;
			return !isEleBlock(sr,sc,tr,tc);
		case B_HORSE:
			if(!((dR==2&&dC==1)||(dR==1&&dC==2))) return false;
			return !isHorseBlock(sr,sc,tr,tc);
		case B_CAR:
			if(dR != 0 && dC != 0) return false;
			return countBlockOnLine(sr,sc,tr,tc) == 0;
		case B_CANNON: {
			if(dR != 0 && dC != 0) return false;
			int blocks = countBlockOnLine(sr,sc,tr,tc);
			if(target == EMPTY) return blocks == 0;
			else return blocks == 1;
		}
		case B_SOL: {
			int offsetR = tr - sr;
			//黑卒未过河 r<=4：行号增大向下走一格
			if(sr <= 4) {
				if(offsetR != 1 || dC != 0) return false;
			} else {
				bool valid = (offsetR == 1 && dC==0) || (offsetR==0 && dC==1);
				if(!valid) return false;
			}
			return true;
		}
		default:
			return false;
	}
}
//找到红帅(r,c)、黑将(r,c)
static void findKing(int &rr,int &rc,int &br,int &bc) {
	rr=rc=br=bc=-1;
	for(int r=0; r<XQ_ROW; r++) {
		for(int c=0; c<XQ_COL; c++) {
			if(board[r][c]==R_KING) {
				rr=r;
				rc=c;
			}
			if(board[r][c]==B_KING) {
				br=r;
				bc=c;
			}
		}
	}
}
// 在指定棋盘上，attackerSide(1红2黑)能否攻击(tr,tc)
static bool isAttackedOn(int map[XQ_ROW][XQ_COL], int attackerSide, int tr, int tc) {
	// tmp 改成同规格二维数组，用来备份全局棋盘
	int tmp[XQ_ROW][XQ_COL];
	for(int r=0; r<XQ_ROW; r++) {
		for(int c=0; c<XQ_COL; c++) {
			int p = map[r][c];
			if(p == EMPTY) continue;
			if(attackerSide == 1 && p > R_SOL) continue;
			if(attackerSide == 2 && p < B_KING) continue;

			copyBoard(tmp, board);      // 备份真实全局棋盘
			copyBoard(board, map);      // 临时替换为模拟棋盘
			bool ok = canMove(r,c,tr,tc);
			copyBoard(board, tmp);      // 还原原本棋盘
			if(ok) return true;
		}
	}
	return false;
}

// 在指定棋盘上，side(1红2黑)是否被将军
static bool isCheckOn(int map[XQ_ROW][XQ_COL], int side) {
	int rr,rc,br,bc;
	int tmp[XQ_ROW][XQ_COL];
	copyBoard(tmp, board);
	copyBoard(board, map);
	findKing(rr,rc,br,bc);
	copyBoard(board, tmp);

	if(side == 1) {
		return isAttackedOn(map, 2, rr, rc);
	} else {
		return isAttackedOn(map, 1, br, bc);
	}
}
//模拟移动后，我方是否仍被将军（非法走棋）
static bool moveStillCheck(int sr,int sc,int tr,int tc,int side) {
	int sim[XQ_ROW][XQ_COL];
	copyBoard(sim, board);
	//临时棋盘模拟走子
	sim[tr][tc] = sim[sr][sc];
	sim[sr][sc] = EMPTY;

	//校验将帅面对面
	int rr,rc,br,bc;
	int backup[XQ_ROW][XQ_COL];
	copyBoard(backup, board);
	copyBoard(board, sim);
	findKing(rr,rc,br,bc);
	bool kingFace = kingFaceEachOther(rr,rc,br,bc);
	copyBoard(board, backup);
	if(kingFace) return true;

	//模拟走完己方被将军：该步走子非法，拦截
	return sideInCheck(sim, side);
}
static int checkButtonClick(int mx, int my) {
	if(hitUndoBtn(mx, my)) return 1;
	if(hitBackBtn(mx, my)) return 2;
	return 0;
}
static bool isCheck(int side) {
	return isCheckOn(board, side);
}
static void checkGameResult() {
	int rr,rc,br,bc;
	findKing(rr,rc,br,bc);
	//规则1：将帅直接对面，走子方判负
	if(kingFaceEachOther(rr,rc,br,bc)) {
		gameOver = true;
		winner = (turn == 1) ? 2 : 1;
		return;
	}
	int enemySide = turn == 1 ? 2 : 1;
	bool enemyCheck = sideInCheck(board, enemySide);
	bool enemyCanEscape = playerHasValidMoves(enemySide);

	//规则2：被将军，且无任何解招 → 将死，进攻方胜利
	if(enemyCheck) {
		if(!enemyCanEscape) {
			gameOver = true;
			winner = turn;
		}
	}
	//规则3：没有被将军、但无子可动 → 困毙（闷宫），进攻方胜利
	else {
		if(!enemyCanEscape) {
			gameOver = true;
			winner = turn;
		}
	}
}
static bool posBeAttacked(int simBoard[XQ_ROW][XQ_COL], int tarR, int tarC, int attackerSide) {
	//临时备份真实棋盘
	int backup[XQ_ROW][XQ_COL];
	copyBoard(backup, board);
	copyBoard(board, simBoard);

	for(int r=0; r<XQ_ROW; r++) {
		for(int c=0; c<XQ_COL; c++) {
			int piece = board[r][c];
			if(piece == EMPTY) continue;
			//筛选对应阵营的进攻棋子：1=红方进攻、2=黑方进攻
			if(attackerSide == 1 && piece > R_SOL) continue;
			if(attackerSide == 2 && piece < B_KING) continue;
			if(canMove(r,c,tarR,tarC)) {
				copyBoard(board, backup);
				return true;
			}
		}
	}
	copyBoard(board, backup);
	return false;
}
static bool sideInCheck(int simBoard[XQ_ROW][XQ_COL], int checkSide) {
	int rr,rc,br,bc;
	int backup[XQ_ROW][XQ_COL];
	copyBoard(backup, board);
	copyBoard(board, simBoard);
	findKing(rr,rc,br,bc);
	copyBoard(board, backup);

	if(checkSide == 1) {
		//红帅被黑方进攻=被将军
		return posBeAttacked(simBoard, rr, rc, 2);
	} else {
		//黑将被红方进攻=被将军
		return posBeAttacked(simBoard, br, bc, 1);
	}
}
static bool playerHasValidMoves(int side) {
	int pieceLow, pieceHigh;
	if(side == 1) {
		pieceLow=R_KING;
		pieceHigh=R_SOL;
	} else {
		pieceLow=B_KING;
		pieceHigh=B_SOL;
	}

	for(int sr=0; sr<XQ_ROW; sr++) {
		for(int sc=0; sc<XQ_COL; sc++) {
			int p = board[sr][sc];
			if(p < pieceLow || p > pieceHigh) continue;
			for(int tr=0; tr<XQ_ROW; tr++) {
				for(int tc=0; tc<XQ_COL; tc++) {
					if(!canMove(sr,sc,tr,tc)) continue;
					if(!moveStillCheck(sr,sc,tr,tc,side)) {
						return true;
					}
				}
			}
		}
	}
	return false;
}
static void drawPreviewChess() {
	// 无选中 / 鼠标不在棋盘 直接退出
	if(selectR == -1 || hoverR == -1) return;
	// 基础走法不合法 不绘制预览
	if(!canMove(selectR, selectC, hoverR, hoverC)) return;
	// 走后会将军/将帅对面 不预览
	if(moveStillCheck(selectR, selectC, hoverR, hoverC, turn)) return;

	int px = XQ_OFFSET + hoverC * XQ_GRID;
	int py = XQ_OFFSET + hoverR * XQ_GRID;
	int pieceType = board[selectR][selectC]; // 修正之前下标错误

	// 浅色底色代替透明
	if(pieceType <= R_SOL)
		setfillcolor(RGB(255, 210, 210)); // 浅粉红
	else
		setfillcolor(RGB(210, 210, 210));  // 浅灰
	setlinecolor(RGB(80,80,80));
	solidcircle(px, py, CHESS_R);
	circle(px, py, CHESS_R);

	// 浅色文字
	if(pieceType <= R_SOL)
		settextcolor(RGB(180, 0, 0));
	else
		settextcolor(RGB(60,60,60));
	settextstyle(18,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(px-9, py-11, chessName[pieceType]);
}
static bool doUndo() {
	if(undoStack.empty()) return false;
	ChessSnapshot last = undoStack.top();
	undoStack.pop();
	copyBoard(board, last.boardData);
	turn = last.turnVal;
	selectR = selectC = -1;
	// 撤销对局结算状态，恢复可下棋
	gameOver = false;
	winner = 0;
	return true;
}

// 象棋对外入口函数
void runXiangqi() {
	resetXqGame();
	int winW = XQ_OFFSET*2 + XQ_COL*XQ_GRID;
	int winH = BTN_Y + BTN_H + 10;
	initgraph(winW,winH);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "中国象棋");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/chchess.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	MOUSEMSG m;
	int gr,gc;
	while (!backMenu) {
		BeginBatchDraw();
		drawBg();
		drawBoard();
		drawAllChess();
		drawPreviewChess();
		settextstyle(18,0,"宋体");
		setbkmode(TRANSPARENT);
		char msg[128];
		if(gameOver) {
			sprintf(msg,"对局结束！%s获胜，点击任意位置重开",winner==1?"红方":"黑方");
			settextcolor(RED);
		} else {
			sprintf(msg,"当前：%s方走棋",turn==1?"红":"黑");
			if(isCheck(turn)) {
				strcat(msg," 【将军！必须应将】");
				settextcolor(RED);
			} else
				settextcolor(BLACK);
		}
		outtextxy(20, BTN_Y-30, msg);
		drawBtn();
		EndBatchDraw();

		m = GetMouseMsg();
		// 对局结束只处理点击重置，其它操作忽略
		if(pos2Grid(m.x, m.y, hoverR, hoverC) == false) {
			hoverR = -1;
			hoverC = -1;
		}
		if(gameOver) {
			// 左键先检测是不是点按钮（悔棋、返回菜单）
			if(m.uMsg == WM_LBUTTONDOWN) {
				int btnId = checkButtonClick(m.x, m.y);
				if(btnId == 1) { // 悔棋按钮ID
					doUndo();
					continue;
				}
				if(btnId == 2) { // 返回主菜单
					backMenu = true;
					break;
				}
				// 点空白棋盘：彻底重置整局游戏
				resetXqGame();
			}
			continue;
		}

		// 只处理左键点击
		if(m.uMsg == WM_LBUTTONDOWN) {
			// 1. 先判断是否点击悔棋按钮
			if(hitUndoBtn(m.x, m.y)) {
				doUndo();
				continue;
			}
			// 2. 判断返回菜单
			if(hitBackBtn(m.x,m.y)) {
				backMenu = true;
				break;
			}
			// 3. 判断是否点棋盘
			if(!pos2Grid(m.x,m.y,gr,gc)) continue;

			if(selectR == -1) {
				// 选中己方棋子
				int piece = board[gr][gc];
				if(piece == EMPTY) continue;
				bool isRedPiece = (piece >= R_KING && piece <= R_SOL);
				bool turnIsRed = (turn == 1);
				if( (turnIsRed && isRedPiece) || (!turnIsRed && !isRedPiece) ) {
					selectR = gr;
					selectC = gc;
				}
			} else {
				// 已有选中，尝试落子
				if(canMove(selectR, selectC, gr, gc)) {
					if(!moveStillCheck(selectR,selectC,gr,gc,turn)) {
						// =====正确顺序：先保存当前局面，再移动棋子=====
						pushSnapshot();
						board[gr][gc] = board[selectR][selectC];
						board[selectR][selectC] = EMPTY;
						checkGameResult();
						turn = (turn == 1) ? 2 : 1;
					}
				}
				selectR = selectC = -1;
			}
		}
	}
	closegraph();
}
