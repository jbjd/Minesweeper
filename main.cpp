#define NOMINMAX
#include <windows.h> 
#include <gdiplus.h>
#include <cstdio>
#include <chrono>
#include <thread>
#include <memory>
#include <iostream>
#include <random>
#include <string>

const int COL = 30;
const int ROW = 16;
const int H = 480;
const int TILESIZE = 30;
const int TOP  = 100;
const int HEADERSIZE = 70;

struct tile{
	bool bomb = false;
	int nextTo = 0;
	bool hidden = true;
	bool flag = false;
};
int bombs = 99;
int revealed = 0;
int tick = 0; // this divided by 50 is time in seconds
tile arr[H];
bool inProgress = false;
bool hasClicked = false;



void countBombs(tile (&arr)[480]){
	arr[0].nextTo = arr[1].bomb+arr[COL].bomb+arr[COL+1].bomb;  // nw corner
	arr[COL-1].nextTo = arr[COL-2].bomb+arr[COL+COL-1].bomb+arr[COL+COL-2].bomb;  // ne corner
	arr[H-COL].nextTo = arr[H-COL-COL].bomb+arr[H-COL+1].bomb+arr[H-COL-COL+1].bomb;  // sw corner
	arr[H-1].nextTo = arr[H-2].bomb+arr[H-COL-1].bomb+arr[H-COL-2].bomb;  // se corner
	// top row
	for(int i = 1; i < COL-1; ++i){
		arr[i].nextTo = arr[i-1].bomb+arr[i+1].bomb+arr[i+COL].bomb+arr[i+COL-1].bomb+arr[i+COL+1].bomb;
	}
	// bot row
	for(int i = H-COL+1; i < H-1; ++i){
		arr[i].nextTo = arr[i-1].bomb+arr[i+1].bomb+arr[i-COL].bomb+arr[i-COL-1].bomb+arr[i-COL+1].bomb;
	}
	// left col
	for(int i = COL; i < H-COL; i+=COL){
		arr[i].nextTo = arr[i-COL].bomb+arr[i-COL+1].bomb+arr[i+1].bomb+arr[i+COL].bomb+arr[i+COL+1].bomb;
	}
	// right col
	for(int i = COL+COL-1; i < H-1; i+=COL){
		arr[i].nextTo = arr[i-COL].bomb+arr[i-COL-1].bomb+arr[i-1].bomb+arr[i+COL].bomb+arr[i+COL-1].bomb;
	}
	// all rows between top/bot
	int ind;
	for(int j = 1; j < ROW-1; ++j){
		//for each tile in row
		for(int i = 1; i < COL-1; ++i){
			ind = i+(j*COL);
			arr[ind].nextTo = arr[ind-1].bomb+arr[ind+1].bomb+arr[ind-COL-1].bomb+arr[ind-COL].bomb+arr[ind-COL+1].bomb+arr[ind+COL-1].bomb+arr[ind+COL].bomb+arr[ind+COL+1].bomb;
		}
	}

	/*for(int i = 0; i < ROW; ++i){
		for(int j = 0; j < COL; ++j){
			if(arr[j+(i*COL)].bomb)
				std::cout << "* ";
			else
				std::cout << arr[j+(i*COL)].nextTo << ' ';
		}
		std::cout << '\n';
	}
	exit(0);*/
}

// Clocks for limiting fps
std::chrono::time_point<std::chrono::steady_clock> nextF = std::chrono::steady_clock::now();

// global vars
std::shared_ptr<Gdiplus::Graphics> graphics;  // gdiplus for drawing
int active = 1;  // if main loop continues
RECT rect; // screen bbox
Gdiplus::Color back = Gdiplus::Color(255, 192, 192, 192);
Gdiplus::Color darkGray = Gdiplus::Color(255, 128, 128, 128);
Gdiplus::Color backer = Gdiplus::Color(255, 184, 185, 184);
Gdiplus::Color white = Gdiplus::Color(255, 240, 240, 240);
Gdiplus::Color black = Gdiplus::Color(255, 1, 1, 1);
Gdiplus::Color red = Gdiplus::Color(255, 230, 50, 10);
Gdiplus::Color yel = Gdiplus::Color(255, 255, 255, 0);
Gdiplus::Color one = Gdiplus::Color(255, 30, 50, 200);
Gdiplus::Color two = Gdiplus::Color(255, 40, 110, 30);
Gdiplus::Color three = Gdiplus::Color(255, 200, 50, 30);
Gdiplus::Color four = Gdiplus::Color(255, 110, 40, 110);
Gdiplus::Color five = Gdiplus::Color(255, 150, 30, 30);
Gdiplus::Color six = Gdiplus::Color(255, 30, 110, 150);
Gdiplus::Color sev = Gdiplus::Color(255, 200, 40, 160);

int topLeftX, bottom;

void getDimensions(HWND w){
	GetWindowRect(w, &rect);
	rect.right -= rect.left;
	rect.bottom -= rect.top;
	topLeftX = ((rect.right-(TILESIZE*30))/2)-1;
	bottom = 200+(TILESIZE*16);
}

void updateTiles(){
	Gdiplus::SolidBrush behind(black);
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font font(&fontFamily, 10, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
	Gdiplus::Pen darkPen(darkGray);
	Gdiplus::Pen whitePen(white);
	Gdiplus::SolidBrush oneMine(one);
	Gdiplus::SolidBrush twoMine(two);
	Gdiplus::SolidBrush threeMine(three);
	Gdiplus::SolidBrush fourMine(four);
	Gdiplus::SolidBrush fiveMine(five);
	Gdiplus::SolidBrush sixMine(six);
	Gdiplus::SolidBrush sevMine(sev);
	Gdiplus::SolidBrush eightMine(black);
	Gdiplus::SolidBrush wiper(back);
	Gdiplus::SolidBrush wiper2(backer);

	Gdiplus::Bitmap bmp(TILESIZE*30, TILESIZE*16);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&wiper, 0, 0, TILESIZE*30, TILESIZE*16);
	std::wstring nearCount;
	for(int j = 0; j < 16; ++j){
		for(int i = 0; i < 30; ++i){
			int x = (i*TILESIZE);
			int y = (j*TILESIZE);
			if(arr[(30*j)+i].hidden){
				graph->DrawLine(&whitePen, x, y, x+TILESIZE-1, y);
				graph->DrawLine(&whitePen, x, y, x, y+TILESIZE-1);
				graph->DrawLine(&darkPen, x+TILESIZE-1, y, x+TILESIZE-1, y+TILESIZE);
				graph->DrawLine(&darkPen, x, y+TILESIZE-1, x+TILESIZE-1, y+TILESIZE-1);
				if(arr[(30*j)+i].flag){
					graph->FillRectangle(&behind, x+7, y+22, (TILESIZE>>1)+1, 3);
					graph->FillRectangle(&behind, x+14, y+9, 2, 13);
					// red flag
					Gdiplus::PointF p1(x+16, y+5);
					Gdiplus::PointF p2(x+16, y+15);
					Gdiplus::PointF p3(x+8, y+10);
					Gdiplus::PointF points[3] ={p1,p2,p3};
					graph->FillPolygon(&threeMine, points, 3);
				}
			}else if(arr[(30*j)+i].bomb){
				Gdiplus::SolidBrush base(black);
				Gdiplus::SolidBrush back(red);
				Gdiplus::Pen lines(black, 2);

				graph->FillRectangle(&back, x, y, TILESIZE-1, TILESIZE-1);
				graph->FillEllipse(&base, x+6, y+6, TILESIZE-13, TILESIZE-13);
				graph->DrawLine(&lines, x+15, y+5, x+15, y+25);
				graph->DrawLine(&lines, x+5, y+15, x+25, y+15);
			}else{
				graph->FillRectangle(&wiper2, x, y, TILESIZE-1, TILESIZE-1);
				if(arr[(30*j)+i].nextTo){
					nearCount = std::to_wstring(arr[(30*j)+i].nextTo);
					Gdiplus::PointF pointF(x+7, y+5);
					switch(arr[(30*j)+i].nextTo){
						case 1:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &oneMine);
							break;
						case 2:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &twoMine);
							break;
						case 3:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &threeMine);
							break;
						case 4:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &fourMine);
							break;
						case 5:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &fiveMine);
							break;
						case 6:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &sixMine);
							break;
						case 7:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &sevMine);
							break;
						default:
							graph->DrawString(nearCount.c_str(), -1, &font, pointF, NULL, &eightMine);
							break;
					}
				}
			}
		}
	}
	
	graphics->DrawImage(&bmp, topLeftX+2, TOP+HEADERSIZE+14, (TILESIZE*30), (TILESIZE*16));
	delete graph;
}

// redraws hidden tile, dont call for revealed tiles
void redrawTile(const int &x, const int &y){
	Gdiplus::SolidBrush wiper(back);
	int xc = topLeftX+2+(x*TILESIZE);
	int yc = TOP+HEADERSIZE+14+(y*TILESIZE);
	// draw gray box and put special stuff on top
	graphics->FillRectangle(&wiper, xc+1, yc+1, TILESIZE-2, TILESIZE-2);

	if(arr[(30*y)+x].bomb && !arr[(30*y)+x].hidden){
		Gdiplus::SolidBrush base(black);
		Gdiplus::SolidBrush back(red);
		Gdiplus::Pen lines(black, 2);

		graphics->FillRectangle(&back, xc, yc, TILESIZE-1, TILESIZE-1);
		graphics->FillEllipse(&base, xc+6, yc+6, TILESIZE-13, TILESIZE-13);
		graphics->DrawLine(&lines, xc+15, yc+5, xc+15, yc+25);
		graphics->DrawLine(&lines, xc+5, yc+15, xc+25, yc+15);
	}
	else if(arr[(30*y)+x].flag){
		Gdiplus::SolidBrush base(black);
		Gdiplus::SolidBrush flag(three);
		graphics->FillRectangle(&base, xc+7, yc+22, (TILESIZE>>1)+1, 3);
		graphics->FillRectangle(&base, xc+14, yc+9, 2, 13);
		// red flag
		Gdiplus::PointF p1(xc+16, yc+5);
		Gdiplus::PointF p2(xc+16, yc+15);
		Gdiplus::PointF p3(xc+8, yc+10);
		Gdiplus::PointF points[3] ={p1,p2,p3};
		graphics->FillPolygon(&flag, points, 3);
	}
}

void addMines(tile (&arr)[480]){
	std::minstd_rand  rng(time(NULL));
	std::uniform_int_distribution<std::minstd_rand0::result_type> dist(0, H-1);
	int total = 0;
	int temp;
	while(total < 99){
		temp = dist(rng);
		if(!arr[temp].bomb){
			arr[temp].bomb = true;
			++total;
		}
	}
}

void resetTiles(tile (&arr)[480]){
	bombs = 99;
	revealed = 0;
	for(int i = 0; i < 480; ++i){
		arr[i].hidden = true;
		arr[i].bomb = false;
		arr[i].nextTo = 0;
		arr[i].flag = false;
	}
	tick = 0;
	inProgress = true;
	hasClicked = false;
	addMines(arr);
	countBombs(arr);
	updateTiles();
}

void drawState(){
	Gdiplus::Bitmap bmp(rect.right,rect.bottom);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	// background
	Gdiplus::SolidBrush wiper(back);
	graph->FillRectangle(&wiper, 0, 0, rect.right, rect.bottom);
	// header
	Gdiplus::Pen darkPen(darkGray);
	graph->DrawLine(&darkPen, topLeftX, TOP, topLeftX, TOP+HEADERSIZE);
	graph->DrawLine(&darkPen, topLeftX, TOP, rect.right-topLeftX, TOP);
	Gdiplus::Pen whitePen(white);
	graph->DrawLine(&whitePen, rect.right-topLeftX, TOP, rect.right-topLeftX, TOP+HEADERSIZE);
	graph->DrawLine(&whitePen, topLeftX, TOP+HEADERSIZE, rect.right-topLeftX, TOP+HEADERSIZE);
	// draw main area
	graph->DrawLine(&darkPen, topLeftX, TOP+HEADERSIZE+12, topLeftX, TOP+HEADERSIZE+12+(16*TILESIZE)+2);
	graph->DrawLine(&darkPen, topLeftX, TOP+HEADERSIZE+12, rect.right-topLeftX, TOP+HEADERSIZE+12);
	graph->DrawLine(&whitePen, rect.right-topLeftX, TOP+HEADERSIZE+12, rect.right-topLeftX, TOP+HEADERSIZE+12+(16*TILESIZE)+2);
	graph->DrawLine(&whitePen, topLeftX, TOP+HEADERSIZE+12+(16*TILESIZE)+2, rect.right-topLeftX, TOP+HEADERSIZE+12+(16*TILESIZE)+2);

	graphics->DrawImage(&bmp, 0, 0, (int)rect.right, (int)rect.bottom);
	delete graph;
	
	updateTiles();
}

void updateHeader(){
	Gdiplus::SolidBrush behind(black);
	Gdiplus::Pen bPen(black);
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font       font(&fontFamily, 20, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
	Gdiplus::PointF     pointF(0, 1);
	Gdiplus::SolidBrush redBrush(red);
	Gdiplus::Pen darkPen(darkGray, 2);
	Gdiplus::Pen whitePen(white);
	Gdiplus::SolidBrush fac(yel);
	// timer
	int time = tick/50;
	std::wstring timer;
	if(time > 999)
		timer = L"999";
	else if(time < 10)
		timer = L"00"+std::to_wstring(time);
	else if(time < 100)
		timer = L"0"+std::to_wstring(time);
	else
		timer = std::to_wstring(time);
	Gdiplus::Bitmap bmp(66, 40);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&behind, 0, 0, 66, 40);
	graph->DrawString(timer.c_str(), -1, &font, pointF, NULL, &redBrush);
	
	graphics->DrawImage(&bmp, rect.right-topLeftX-86, TOP+15, 66, 40);
	delete graph;
	// bomb count
	std::wstring bombCount;
	bombCount = bombs > 9 || bombs < 0 ? L"0"+std::to_wstring(bombs) : L"00"+std::to_wstring(bombs);
	if(bombs > 9)
		bombCount = L"0"+std::to_wstring(bombs);
	else if(bombs >= 0)
		bombCount = L"00"+std::to_wstring(bombs);
	else if(bombs >= -9)
		bombCount = L"-0"+std::to_wstring(-1*bombs);
	else if(bombs >= -99)
		bombCount = L"-"+std::to_wstring(-1*bombs);
	else
		bombCount = L"---";
	graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&behind, 0, 0, 66, 40);
	graph->DrawString(bombCount.c_str(), -1, &font, pointF, NULL, &redBrush);
	
	graphics->DrawImage(&bmp, topLeftX+20, TOP+15, 66, 40);
	delete graph;
	Gdiplus::Bitmap face(40, 40);

	graph = Gdiplus::Graphics::FromImage(&face);
	graph->DrawRectangle(&darkPen, 0, 0, 39, 39);
	graph->DrawLine(&whitePen, 1, 1, 1, 38);
	graph->DrawLine(&whitePen, 1, 1, 38, 1);
	// face
	graph->FillEllipse(&behind, 9.0F, 9.0F, 22.0F, 22.0F);
	graph->FillEllipse(&fac, 10.0F, 10.0F, 20.0F, 20.0F);
	graph->FillRectangle(&behind, 16, 16, 2, 2);
	graph->FillRectangle(&behind, 23, 16, 2, 2);
	//mouth
	graph->DrawArc(&bPen, 15, 20, 10, 5, 180, -180);
	
	graphics->DrawImage(&face, ((int)(rect.right/2))-20, TOP+15, 40, 40);
	delete graph;
}


void showNear(int x, int y, bool nw=true, bool n=true, bool ne=true, bool w=true, bool e=true, bool sw=true, bool s=true, bool se=true){
	if(y > 0 && n && arr[(30*(y-1))+x].hidden){
		arr[(30*(y-1))+x].hidden = false;
		++revealed;
		if(!arr[(30*(y-1))+x].nextTo){
			showNear(x, y-1, nw=true, n=true, ne=true);
		}
	}
	if(y < 15 && s && arr[(30*(y+1))+x].hidden){
		arr[(30*(y+1))+x].hidden = false;
		++revealed;
		if(!arr[(30*(y+1))+x].nextTo){
			showNear(x, y+1, sw=true, s=true, se=true);
		}
	}
	if(x > 0 && w && arr[(30*(y))+x-1].hidden){
		arr[(30*(y))+x-1].hidden = false;
		++revealed;
		if(!arr[(30*(y))+x-1].nextTo){
			showNear(x-1, y, nw=true, w=true, sw=true);
		}
	}
	if(x < 29 && e && arr[(30*(y))+x+1].hidden){
		arr[(30*(y))+x+1].hidden = false;
		++revealed;
		if(!arr[(30*(y))+x+1].nextTo){
			showNear(x+1, y, ne=true, e=true, se=true);
		}
	}
	if(y > 0 && x > 0 && nw && arr[(30*(y-1))+x-1].hidden){
		arr[(30*(y-1))+x-1].hidden = false;
		++revealed;
		if(!arr[(30*(y-1))+x-1].nextTo){
			showNear(x-1, y-1, nw=true, n=true, w=true);
		}
	}
	if(y > 0 && x < 29 && ne && arr[(30*(y-1))+x+1].hidden){
		arr[(30*(y-1))+x+1].hidden = false;
		++revealed;
		if(!arr[(30*(y-1))+x+1].nextTo){
			showNear(x+1, y-1, ne=true, n=true, e=true);
		}
	}
	if(y < 15 && x > 0 && sw && arr[(30*(y+1))+x-1].hidden){
		arr[(30*(y+1))+x-1].hidden = false;
		++revealed;
		if(!arr[(30*(y+1))+x-1].nextTo){
			showNear(x-1, y+1, sw=true, s=true, w=true);
		}
	}
	if(y < 15 && x < 29 && se && arr[(30*(y+1))+x+1].hidden){
		arr[(30*(y+1))+x+1].hidden = false;
		++revealed;
		if(!arr[(30*(y+1))+x+1].nextTo){
			showNear(x+1, y+1, se=true, s=true, e=true);
		}
	}
}

//handles events sent to window
LRESULT CALLBACK windowProc(_In_ HWND hwnd, _In_ UINT uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
	int x, y;
	switch(uMsg){
		// resize
		case WM_SIZE:
			// read size of screen and draw background
			getDimensions(hwnd);
			if(graphics == nullptr)
				return 0;
			drawState();
			updateHeader();
			return 0;
		// exit
		case WM_CLOSE:
		case WM_DESTROY:
			active = 0;
			PostQuitMessage(0);
			return 0;
		case WM_PAINT:
			return 0;
		case WM_GETMINMAXINFO:
			lpMMI->ptMinTrackSize.x = 1000;
			lpMMI->ptMinTrackSize.y = 800;
		case WM_LBUTTONDOWN:
			if(!wParam)
				return 0;
			x = (lParam & 0xFFFF);
			y = (lParam >> 16);
			// check if user clicked on new game
			// ((int)(rect.right/2))-20, TOP+15, 40, 40
			if(x >= ((int)(rect.right/2))-20 && x <= ((int)(rect.right/2))+20 && y >= TOP+15 && y <= TOP+55){
				// must have clicked on face
				resetTiles(arr);
			}

			// throw out clicks not on tiles
			if(x <= topLeftX || x >= rect.right-topLeftX ||
			   y <= TOP+HEADERSIZE+14 || y >= TOP+HEADERSIZE+14+(TILESIZE*16))
				return 0;
			// can't click tiles if game ended
			if(!inProgress) return 0;
			x = (x-topLeftX+1)/TILESIZE;
			y = (y-(TOP+HEADERSIZE+15))/TILESIZE;
			// clicked on col x, row y
			if(arr[(30*y)+x].hidden){
				// stop game if mine, else reveal tiles
				if(arr[(30*y)+x].bomb){
					if(!hasClicked){
						// if users first click was a mine, move mine to upper left
						for(int i = 0; i < 400; ++i){
							if(!arr[i].bomb){
								arr[i].bomb = true;
								break;
							}
						}
						arr[(30*y)+x].bomb = false;
						// recount mines 
						countBombs(arr);
						// act as if nothing happened
						goto noMine;
					}
					arr[(30*y)+x].hidden = false;
					inProgress = false;
					redrawTile(x, y);
				}else{
				noMine:
					arr[(30*y)+x].hidden = false;
					++revealed;
					if(!arr[(30*y)+x].nextTo) 
						showNear(x, y);
						updateTiles();
					if(revealed >= 381){
						inProgress = false;
						bombs = 0;
					}
					//std::cout << revealed << \n <<;
				}
			}
			hasClicked = true;
			return 0;
		case WM_RBUTTONDOWN:
			if(!wParam || !inProgress)
				return 0;
			x = (lParam & 0xFFFF);
			y = (lParam >> 16);
			if(x <= topLeftX || x >= rect.right-topLeftX ||
			   y <= TOP+HEADERSIZE+14 || y >= TOP+HEADERSIZE+14+(TILESIZE*16))
				return 0;
			x = (x-topLeftX+1)/TILESIZE;
			y = (y-(TOP+HEADERSIZE+15))/TILESIZE;
			if(!arr[(30*y)+x].hidden || x > 29 || y > 15) return 0;
			// toggle flag and redraw
			if(arr[(30*y)+x].flag){
				++bombs;
			}else{
				--bombs;
			}
			arr[(30*y)+x].flag = !arr[(30*y)+x].flag;
			redrawTile(x, y);
			
			return 0;
		default: 
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

//main function
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine ,int nShowCmd){
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
	WNDCLASS windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpszClassName = "Game Window";
	windowClass.lpfnWndProc = windowProc;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClass(&windowClass);
	HWND window = CreateWindowA(windowClass.lpszClassName, "MineSweeper",  WS_VISIBLE | WS_MAXIMIZE | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, 0, 0, hInstance, 0);
	HDC hdc = GetDC(window);
	ShowWindow(window, SW_MAXIMIZE);

	MSG message; //gets messages sent to window and reads them

	// read size of screen into rect
	getDimensions(window);

	// tools to draw on screen
	graphics = std::make_shared<Gdiplus::Graphics>(hdc);
	Gdiplus::SolidBrush wiper(back);

	graphics->FillRectangle(&wiper, 0, 0, rect.right, rect.bottom);
	drawState();
	
	updateHeader();
	
	for (int i = 0; i < H; ++i) {
		arr[i] = tile();
	}
	// mark 99 random tiles as bombs
	addMines(arr);
	// count how many bombs are next to each tile
	countBombs(arr);
	inProgress = true;
	while (active) {
			// FPS Limiter
			nextF += std::chrono::milliseconds(20); 
			std::this_thread::sleep_until(nextF);
			if(inProgress&&hasClicked) ++tick;
			if(PeekMessage(&message, NULL, 0, 0, PM_REMOVE | PM_NOYIELD)){//this while loop processes message for window
				TranslateMessage(&message); 
				DispatchMessage(&message);  
			}
			// FPS Limiter End

			//CODE HERE
			updateHeader();
			
	}

	return message.wParam; // Checks most recent message for return
}
