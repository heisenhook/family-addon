#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <vector>
#include <iomanip>
#include <iostream>

#undef ERROR

enum eLogType : int {
	INFO = 0,
	WARNING,
	ERR,
	DEBUG
};

enum eLogColor {
	FORE_BLUE = FOREGROUND_BLUE,
	FORE_GREEN = FOREGROUND_GREEN,
	FORE_RED = FOREGROUND_RED,
	FORE_INTENSITY = FOREGROUND_INTENSITY,
	FORE_GRAY = FOREGROUND_INTENSITY,
	FORE_CYAN = FOREGROUND_BLUE | FOREGROUND_GREEN,
	FORE_MAGENTA = FOREGROUND_BLUE | FOREGROUND_RED,
	FORE_YELLOW = FOREGROUND_GREEN | FOREGROUND_RED,
	FORE_BLACK = 0U,
	FORE_WHITE = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,

	BACK_BLUE = BACKGROUND_BLUE,
	BACK_GREEN = BACKGROUND_GREEN,
	BACK_RED = BACKGROUND_RED,
	BACK_INTENSITY = BACKGROUND_INTENSITY,
	BACK_GRAY = BACKGROUND_INTENSITY,
	BACK_CYAN = BACKGROUND_BLUE | BACKGROUND_GREEN,
	BACK_MAGENTA = BACKGROUND_BLUE | BACKGROUND_RED,
	BACK_YELLOW = BACKGROUND_GREEN | BACKGROUND_RED,
	BACK_BLACK = 0U,
	BACK_WHITE = BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE,

	/* [internal] */
	DEFAULT = FORE_WHITE | BACK_BLACK
};

class Logger {
private:
	HANDLE hConsole;
	std::vector<std::string> logs;
public:
	class stream {
	private:
		bool firstLine = true;
	public:
		HANDLE hConsole;
		int oPrecision;
		int precision = -1;

		void output(std::string out) {
			WriteConsoleA(hConsole, out.c_str(), strlen(out.c_str()), nullptr, nullptr);
		}

		bool setOutputColor(const eLogColor color) {
			return SetConsoleTextAttribute(hConsole, color);
		}

		stream& operator()(const eLogType type = INFO) {
			//setOutputColor()
			if (firstLine)
				firstLine = false;
			else
				*this << "\n";

			return *this;
		}

		stream& operator<<(const char* cstr) {
			if (cstr == "" || cstr == nullptr)
				return *this;

			output(cstr);
			return *this;
		}

		stream& operator<<(std::string str) {
			output(str);
			return *this;
		}

		stream& operator<<(int i) {
			output(std::to_string(i));
			return *this;
		}

		stream& operator<<(unsigned int i) {
			output(std::to_string(i));
			return *this;
		}

		stream& operator<<(float f) {
			if (precision = -1)
				oPrecision = precision = std::cout.precision();
			std::setprecision(precision);
			output(std::to_string(f));
			std::setprecision(oPrecision);
			return *this;
		}

		stream& operator<<(double d) {
			if (precision = -1)
				oPrecision = precision = std::cout.precision();
			std::setprecision(precision);
			output(std::to_string(d));
			std::setprecision(oPrecision);
			return *this;
		}

		stream& operator<<(void* ptr) {
			output(std::format("0x{:X} -> 0x{:X}", (DWORD)&ptr, (DWORD)ptr));
			return *this;
		}

		void setprecision(int i) {
			precision = i;
		}

		int getprecision() {
			return precision;
		}
	};
	stream stream;

	bool attach(std::string title) {
		AllocConsole();
		SetConsoleTitleA(title.c_str());
		freopen_s(reinterpret_cast<FILE**> stdout, "CONOUT$", "w", stdout);

		hConsole = GetStdHandle(
			STD_OUTPUT_HANDLE);
		SetConsoleMode(hConsole, ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		stream.hConsole = hConsole;
		return true;
	}

	bool detach() {
		fclose(stdout);
		return FreeConsole();
	}

	void setprecision(int i) {
		stream.setprecision(i);
	}

	int getprecision() {
		return stream.getprecision();
	}
}; inline Logger gLogger;

#define Log(type) gLogger.stream(type)

#define ERROR