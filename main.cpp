// g++ main.cpp -mwindows -o minesweeper -lgdiplus
#define NOMINMAX
#include <windows.h> 
#include <gdiplus.h>
#include <chrono>
#include <memory>
//#include <iostream>

const int TILESIZE = 30, 
COL = 30, ROW = 16,
H = 480,
TOP = 100,
HEADERSIZE = 70,
WID=COL*TILESIZE, HEI=ROW*TILESIZE;

struct tile{
	int nextTo = 0;
	bool hidden = true,
	bomb = false,
	flag = false;
};
int bombs = 99, revealed = 0, // total mines and spaces revealed
tick = 0, // this divided by 50 is time in seconds
topLeftX, bottom,  // location of box on screen
shuffledArray[H];  //shuffled and used for mine placement
tile arr[H];
bool inProgress = false, hasClicked = false;  // if timer should run and if user has clicked in current game


void countBombs(tile (&arr)[H]){
	arr[0].nextTo = arr[1].bomb+arr[COL].bomb+arr[COL+1].bomb;  // nw corner
	arr[COL-1].nextTo = arr[COL-2].bomb+arr[COL+COL-1].bomb+arr[COL+COL-2].bomb;  // ne corner
	arr[H-COL].nextTo = arr[H-COL-COL].bomb+arr[H-COL+1].bomb+arr[H-COL-COL+1].bomb;  // sw corner
	arr[H-1].nextTo = arr[H-2].bomb+arr[H-COL-1].bomb+arr[H-COL-2].bomb;  // se corner
	// top row
	int i, j, ind;
	for(i = 1; i < COL-1; ++i){
		arr[i].nextTo = arr[i-1].bomb+arr[i+1].bomb+arr[i+COL].bomb+arr[i+COL-1].bomb+arr[i+COL+1].bomb;
	}
	// bot row
	for(i = H-COL+1; i < H-1; ++i){
		arr[i].nextTo = arr[i-1].bomb+arr[i+1].bomb+arr[i-COL].bomb+arr[i-COL-1].bomb+arr[i-COL+1].bomb;
	}
	// left col
	for(i = COL; i < H-COL; i+=COL){
		arr[i].nextTo = arr[i-COL].bomb+arr[i-COL+1].bomb+arr[i+1].bomb+arr[i+COL].bomb+arr[i+COL+1].bomb;
	}
	// right col
	for(i = COL+COL-1; i < H-1; i+=COL){
		arr[i].nextTo = arr[i-COL].bomb+arr[i-COL-1].bomb+arr[i-1].bomb+arr[i+COL].bomb+arr[i+COL-1].bomb;
	}
	// all rows between top/bot
	for(j = 1; j < ROW-1; ++j){
		//for each tile in row
		for(i = 1; i < COL-1; ++i){
			ind = i+(j*COL);
			arr[ind].nextTo = arr[ind-1].bomb+arr[ind+1].bomb+arr[ind-COL-1].bomb+arr[ind-COL].bomb+arr[ind-COL+1].bomb+arr[ind+COL-1].bomb+arr[ind+COL].bomb+arr[ind+COL+1].bomb;
		}
	}
}

// Clocks for limiting fps
using std::chrono::steady_clock;
std::chrono::time_point<steady_clock> nextF = steady_clock::now();

// global vars
std::unique_ptr<Gdiplus::Graphics> graphics;  // gdiplus for drawing
bool active = 1;  // if main loop continues
tagRECT rect; // screen bbox
Gdiplus::Color back = Gdiplus::Color(255, 192, 192, 192),
darkGray = Gdiplus::Color(255, 128, 128, 128),
backer = Gdiplus::Color(255, 184, 185, 184),
white = Gdiplus::Color(255, 240, 240, 240),
black = Gdiplus::Color(255, 1, 1, 1),
red = Gdiplus::Color(255, 230, 50, 10),
yel = Gdiplus::Color(255, 255, 255, 0),
one = Gdiplus::Color(255, 30, 50, 200),
two = Gdiplus::Color(255, 40, 110, 30),
three = Gdiplus::Color(255, 200, 50, 30),
four = Gdiplus::Color(255, 110, 40, 110),
five = Gdiplus::Color(255, 150, 30, 30),
six = Gdiplus::Color(255, 30, 110, 150),
sev = Gdiplus::Color(255, 200, 40, 160);

void getDimensions(const HWND &w){
	GetWindowRect(w, &rect);
	rect.right -= rect.left;
	rect.bottom -= rect.top;
	topLeftX = ((rect.right-(TILESIZE*30))>>1)-1;
	bottom = 200+(TILESIZE*16);
}

void updateTiles(){
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font font(&fontFamily, 10, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
	Gdiplus::Pen darkPen(darkGray), whitePen(white);
	Gdiplus::SolidBrush behind(black), oneMine(one), twoMine(two), threeMine(three), fourMine(four),
	fiveMine(five), sixMine(six), sevMine(sev), eightMine(black), wiper(back), wiper2(backer);

	Gdiplus::Bitmap bmp(WID, HEI);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&wiper, 0, 0, WID, HEI);
	int x, y, ind;
	for(y = 0; y < HEI; y+=TILESIZE){
		for(x = 0; x < WID; x+=TILESIZE){
			ind = y+(x/TILESIZE);
			if(arr[ind].hidden){
				graph->DrawLine(&whitePen, x, y, x+TILESIZE-1, y);
				graph->DrawLine(&whitePen, x, y, x, y+TILESIZE-1);
				graph->DrawLine(&darkPen, x+TILESIZE-1, y, x+TILESIZE-1, y+TILESIZE);
				graph->DrawLine(&darkPen, x, y+TILESIZE-1, x+TILESIZE-1, y+TILESIZE-1);
				if(arr[ind].flag){
					graph->FillRectangle(&behind, x+7, y+22, (TILESIZE>>1)+1, 3);
					graph->FillRectangle(&behind, x+14, y+9, 2, 13);
					// red flag
					Gdiplus::PointF p1(x+16, y+5), p2(x+16, y+15), p3(x+8, y+10),
					points[3] = {p1,p2,p3};
					graph->FillPolygon(&threeMine, points, 3);
				}
			}else if(arr[ind].bomb){
				Gdiplus::SolidBrush base(black), back(red);
				Gdiplus::Pen lines(black, 2);

				graph->FillRectangle(&back, x, y, TILESIZE-1, TILESIZE-1);
				graph->FillEllipse(&base, x+6, y+6, TILESIZE-13, TILESIZE-13);
				graph->DrawLine(&lines, x+15, y+5, x+15, y+25);
				graph->DrawLine(&lines, x+5, y+15, x+25, y+15);
			}else{
				graph->FillRectangle(&wiper2, x, y, TILESIZE-1, TILESIZE-1);
				Gdiplus::PointF pointF(x+7, y+5);
				switch(arr[ind].nextTo){
					case 1:
						graph->DrawString(L"1", 1, &font, pointF, &oneMine);
						break;
					case 2:
						graph->DrawString(L"2", 1, &font, pointF, &twoMine);
						break;
					case 3:
						graph->DrawString(L"3", 1, &font, pointF, &threeMine);
						break;
					case 4:
						graph->DrawString(L"4", 1, &font, pointF, &fourMine);
						break;
					case 5:
						graph->DrawString(L"5", 1, &font, pointF, &fiveMine);
						break;
					case 6:
						graph->DrawString(L"6", 1, &font, pointF, &sixMine);
						break;
					case 7:
						graph->DrawString(L"7", 1, &font, pointF, &sevMine);
						break;
					case 8:
						graph->DrawString(L"8", 1, &font, pointF, &eightMine);
						break;
					default:
						break;
				}
			}
		}
	}
	
	graphics->DrawImage(&bmp, topLeftX+2, TOP+HEADERSIZE+14, (TILESIZE*30), (TILESIZE*16));
	delete graph;
}

// redraws hidden tile, dont call for revealed tiles
void redrawTile(const int &x, const int &y){
	Gdiplus::SolidBrush wiper(back), base(black);
	int xc = topLeftX+2+(x*TILESIZE),
	yc = TOP+HEADERSIZE+14+(y*TILESIZE),
	ind = (30*y)+x;
	// draw gray box and put special stuff on top
	graphics->FillRectangle(&wiper, xc+1, yc+1, TILESIZE-2, TILESIZE-2);

	if(arr[ind].bomb && !arr[ind].hidden){
		Gdiplus::SolidBrush back(red);
		Gdiplus::Pen lines(black, 2);

		graphics->FillRectangle(&back, xc, yc, TILESIZE-1, TILESIZE-1);
		graphics->FillEllipse(&base, xc+6, yc+6, TILESIZE-13, TILESIZE-13);
		graphics->DrawLine(&lines, xc+15, yc+5, xc+15, yc+25);
		graphics->DrawLine(&lines, xc+5, yc+15, xc+25, yc+15);
	}
	else if(arr[ind].flag){
		Gdiplus::SolidBrush flag(three);
		graphics->FillRectangle(&base, xc+7, yc+22, (TILESIZE>>1)+1, 3);
		graphics->FillRectangle(&base, xc+14, yc+9, 2, 13);
		// red flag
		Gdiplus::PointF p1(xc+16, yc+5), p2(xc+16, yc+15), p3(xc+8, yc+10),
		points[3] ={p1,p2,p3};
		graphics->FillPolygon(&flag, points, 3);
	}
}

void shuffle(int (&shuffledArray)[H]){
	srand(time(0));
	int i, ind, temp;
	for (i = 0; i < H; ++i){
		ind = rand() % H;
		temp = shuffledArray[ind];
		shuffledArray[ind] = shuffledArray[i];
		shuffledArray[i] = temp;
	}
}

void addMines(tile (&arr)[H], int (&shuffledArray)[H]){
	shuffle(shuffledArray);
	for(int i = 0; i < 99; ++i){
		arr[shuffledArray[i]].bomb = true;
	}
}

// adds mines and gives each tile count of nearby mines
void setup(tile (&arr)[H]){
	addMines(arr, shuffledArray);
	countBombs(arr);
}

void resetTiles(tile (&arr)[H]){
	bombs = 99;
	revealed = tick = hasClicked = 0;
	for(int i = 0; i < H; ++i){
		arr[i].hidden = true;
		arr[i].bomb = arr[i].flag = false;
		arr[i].nextTo = 0;
	}
	inProgress = true;
	setup(arr);
	updateTiles();
}

void drawState(){
	Gdiplus::Bitmap bmp(rect.right,rect.bottom);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	// background
	Gdiplus::SolidBrush wiper(back);
	graph->FillRectangle(&wiper, 0, 0, rect.right, rect.bottom);
	// header
	Gdiplus::Pen darkPen(darkGray), whitePen(white);
	graph->DrawLine(&darkPen, topLeftX, TOP, topLeftX, TOP+HEADERSIZE);
	graph->DrawLine(&darkPen, topLeftX, TOP, rect.right-topLeftX, TOP);
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
	Gdiplus::SolidBrush behind(black), redBrush(red), fac(yel);
	Gdiplus::Pen bPen(black), darkPen(darkGray, 2), whitePen(white);
	Gdiplus::FontFamily fontFamily(L"Arial");
	Gdiplus::Font font(&fontFamily, 20, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
	Gdiplus::PointF pointF(0, 1);
	
	// timer
	unsigned time = tick/50, temp;
	wchar_t timer[4];
	if(time < 10){
		swprintf(timer, L"00%d\0", time);
	}else if(time < 100){
		swprintf(timer, L"0%d\0", time);
	}else{
		swprintf(timer, L"%d\0", time);
	}
	Gdiplus::Bitmap bmp(66, 40);
	Gdiplus::Graphics* graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&behind, 0, 0, 66, 40);
	graph->DrawString(timer, 3, &font, pointF, &redBrush);
	
	graphics->DrawImage(&bmp, rect.right-topLeftX-86, TOP+15, 66, 40);
	delete graph;

	wchar_t bombCount[4];
	if(bombs > 9){
		swprintf(bombCount, L"0%d\0", bombs);
	}else if(bombs >= 0){
		swprintf(bombCount, L"00%d\0", bombs);
	}else if(bombs >= -9){
		swprintf(bombCount, L"-0%d\0", -1*bombs);
	}else if(bombs >= -99){
		swprintf(bombCount, L"%d\0", bombs);
	}else{
		swprintf(bombCount, L"---\0");
	}
	graph = Gdiplus::Graphics::FromImage(&bmp);
	graph->FillRectangle(&behind, 0, 0, 66, 40);
	graph->DrawString(bombCount, 3, &font, pointF, &redBrush);
	
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
	
	graphics->DrawImage(&face, (rect.right>>1)-20, TOP+15, 40, 40);
	delete graph;
}

void showNear(const int &x, const int &y);
void checkZero(const int &x, const int &y, const int &ind){
	if(arr[ind].hidden){
		arr[ind].hidden = false;
		++revealed;
		if(arr[ind].nextTo == 0){
			showNear(x, y);
		}
	}
}

void showNear(const int &x, const int &y){
	if(y > 0){
		checkZero(x, y-1, (30*(y-1))+x);
		if(x > 0) checkZero(x-1, y-1, (30*(y-1))+x-1);
		if(x < 29) checkZero(x+1, y-1, (30*(y-1))+x+1);
	}
	if(y < 15){
		checkZero(x, y+1, (30*(y+1))+x);
		if(x > 0) checkZero(x-1, y+1, (30*(y+1))+x-1);
		if(x < 29) checkZero(x+1, y+1, (30*(y+1))+x+1);
	}
	if(x > 0){
		checkZero(x-1, y, (30*(y))+x-1);
	}
	if(x < 29){
		checkZero(x+1, y, (30*(y))+x+1);
	}
}

//handles events sent to window
long long __stdcall windowProc(_In_ HWND__* hwnd, _In_ unsigned uMsg, _In_ WPARAM wParam, _In_ LPARAM lParam) {
	LPMINMAXINFO lpMMI = (LPMINMAXINFO)lParam;
	int x, y, index;
	switch(uMsg){
		// resize
		case WM_SIZE:
			// read size of screen and draw background
			getDimensions(hwnd);
			if(graphics != nullptr){
				drawState();
				updateHeader();
			}
			return 0;
		// exit
		case WM_CLOSE:
		case WM_DESTROY:
			active = 0;
			return 0;
		case WM_GETMINMAXINFO:
			lpMMI->ptMinTrackSize.x = 1000;
			lpMMI->ptMinTrackSize.y = 800;
		case WM_LBUTTONDOWN:
			if(!wParam) return 0;
			x = (lParam & 0xFFFF);
			y = (lParam >> 16);
			// check if user clicked on new game
			if(x >= (rect.right>>1)-20 && x <= (rect.right>>1)+19 && y >= TOP+15 && y <= TOP+55){
				// must have clicked on face
				resetTiles(arr);
			}

			// throw out clicks not on tiles
			if(x <= topLeftX || x >= rect.right-topLeftX ||
			y <= TOP+HEADERSIZE+14 || y >= TOP+HEADERSIZE+14+(TILESIZE*16))
				return 0;
			// can't click tiles if game ended
			if(!inProgress) return 0;
			x = (x-topLeftX-2)/TILESIZE;
			y = (y-(TOP+HEADERSIZE+14))/TILESIZE;
			if(x > 29 || y > 15) return 0;
			// clicked on col x, row y
			index = (30*y)+x;
			if(arr[index].hidden && !arr[index].flag){
				// stop game if mine, else reveal tiles
				if(arr[index].bomb){
					if(!hasClicked){
						// if users first click was a mine, move mine to upper left
						for(int i = 0; i < 400; ++i){
							if(!arr[i].bomb){
								arr[i].bomb = true;
								break;
							}
						}
						arr[index].bomb = false;
						// recount mines 
						countBombs(arr);
						// act as if nothing happened
						goto noMine;
					}
					arr[index].hidden = false;
					inProgress = false;
					redrawTile(x, y);
				}else{
				noMine:
					arr[index].hidden = false;
					if(!arr[index].nextTo){
						showNear(x, y);
					}
					updateTiles();
					if(++revealed >= 381){
						inProgress = false;
						bombs = 0;
					}
				}
			}
			hasClicked = true;
			return 0;
		case WM_RBUTTONDOWN:
			if(!wParam || !inProgress) return 0;
			x = (lParam & 0xFFFF);
			y = (lParam >> 16);
			if(x <= topLeftX || x >= rect.right-topLeftX ||
			y <= TOP+HEADERSIZE+14 || y >= TOP+HEADERSIZE+14+(TILESIZE*16))
				return 0;
			x = (x-topLeftX-2)/TILESIZE;
			y = (y-(TOP+HEADERSIZE+14))/TILESIZE;
			index = (30*y)+x;
			if(!arr[index].hidden || x > 29 || y > 15) return 0;
			// toggle flag and redraw
			arr[index].flag ? ++bombs : --bombs;
			
			arr[index].flag = !arr[index].flag;
			redrawTile(x, y);
			
			return 0;
		default: 
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}

//main function
int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nShowCmd){
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	unsigned long long gdiplusToken;
	Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, 0);
	tagWNDCLASSA windowClass = {};
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpszClassName = "Game Window";
	windowClass.lpfnWndProc = windowProc;
	windowClass.hCursor = LoadCursorA(0, IDC_ARROW);
	RegisterClassA(&windowClass);
	HWND__ *window = CreateWindowExA(0L, windowClass.lpszClassName, "MineSweeper",  WS_VISIBLE | WS_MAXIMIZE | WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
	HDC__ *hdc = GetDC(window);
	ShowWindow(window, SW_MAXIMIZE);

	tagMSG message; //gets messages sent to window and reads them

	// read size of screen into rect
	getDimensions(window);

	// tools to draw on screen
	graphics = std::make_unique<Gdiplus::Graphics>(hdc);
	Gdiplus::SolidBrush wiper(back);

	graphics->FillRectangle(&wiper, 0, 0, rect.right, rect.bottom);
	drawState();

	updateHeader();

	for (int i = 0; i < H; ++i) {
		arr[i] = tile();
		shuffledArray[i] = i;
	}

	setup(arr);

	inProgress = true;
	while (active) {
		// FPS Limiter
		nextF += std::chrono::milliseconds(20); 
		long long sleepFor = std::chrono::duration_cast<std::chrono::milliseconds>(nextF - steady_clock::now()).count();
		if(sleepFor > 0){
			Sleep(sleepFor);
		}
		if(inProgress&&hasClicked&&tick<(999*50)) ++tick;
		if(PeekMessage(&message, window, 0, 0, PM_REMOVE | PM_NOYIELD)){//this while loop processes message for window
			TranslateMessage(&message); 
			DispatchMessage(&message);  
		}
		updateHeader();
	}

	return message.wParam; // Checks most recent message for return
}
