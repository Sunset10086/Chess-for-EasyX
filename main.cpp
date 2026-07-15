//代码作者：东风九号
//使用AI辅助编程 

#include <graphics.h>
#include <easyx.h>
#include <windows.h>
// 外部游戏入口声明
extern void runGobang();
extern void runGo();
extern void runXiangqi();
extern void runChess();
extern void runFootball();
#define MENU_W 500
#define MENU_H 520
void drawMenu() {
	setfillcolor(RGB(220,220,220));
	solidrectangle(0,0,MENU_W,MENU_H);
	// 标题
	settextcolor(RGB(180,0,0));
	settextstyle(36,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(160,40,"棋类游戏大厅");
	// 按钮1 五子棋
	setfillcolor(RGB(100,180,255));
	solidrectangle(150,120,350,180);
	settextcolor(BLACK);
	settextstyle(24,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(190,140,"五子棋");
	// 按钮2 围棋
	setfillcolor(RGB(120,220,120));
	solidrectangle(150,200,350,260);
	settextcolor(BLACK);
	settextstyle(24,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(200,220,"围棋");
// 象棋按钮
	setfillcolor(RGB(240,170,80));
	solidrectangle(150,280,350,340);
	settextcolor(BLACK);
	settextstyle(24,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(192,300,"中国象棋");
	// 国际象棋按钮
	setfillcolor(RGB(190,130,220));
	solidrectangle(150,360,350,420);
	settextcolor(BLACK);
	settextstyle(24,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(185,380,"国际象棋");
	//高仕足球棋按钮
	setfillcolor(RGB(50,160,80));
	solidrectangle(150,440,350,500);
	settextcolor(BLACK);
	settextstyle(24,0,"宋体");
	setbkmode(TRANSPARENT);
	outtextxy(172,460,"高仕足球棋");
}
int main() {
	initgraph(MENU_W, MENU_H);
	HWND hwnd = GetHWnd(); // EasyX获取窗口句柄
	SetWindowText(hwnd, "棋类游戏大厅");
	HICON hIcon = (HICON)LoadImage(NULL, "Icons/chessboard.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
	MOUSEMSG m;
	while(1) {
		BeginBatchDraw();
		drawMenu();
		EndBatchDraw();
		m = GetMouseMsg();
		if(m.uMsg == WM_LBUTTONDOWN) {
			// 五子棋按钮
			if(m.x >=150 && m.x <=350 && m.y >=120 && m.y <=180) {
				closegraph();
				runGobang();
				initgraph(MENU_W, MENU_H);
			}
			// 围棋按钮
			else if(m.x >=150 && m.x <=350 && m.y >=200 && m.y <=260) {
				closegraph();
				runGo();
				initgraph(MENU_W, MENU_H);
				//象棋按钮
			} else if(m.x >=150 && m.x <=350 && m.y >=280 && m.y <=340) {
				closegraph();
				runXiangqi();
				initgraph(MENU_W, MENU_H);
				//国际象棋 
			} else if(m.x >=150 && m.x <=350 && m.y >=360 && m.y <=420) {
				closegraph();
				runChess();
				initgraph(MENU_W, MENU_H);
			//高仕足球棋
			} else if(m.x >=150 && m.x <=350 && m.y >=440 && m.y <=500) {
				closegraph();
				runFootball();
				initgraph(MENU_W, MENU_H);
			}
		}
	}
	closegraph();
	return 0;
}
