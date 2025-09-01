#pragma once

class Tab;
class Element;
class Form {
public:
	/*0x00*/double m_opacity;
private:
	/*0x08*/ int one;
	/*0x0C*/ int two;
	/*0x10*/ int three;
public:
	/*0x14*/ int m_x;
	/*0x18*/ int m_y;
	/*0x1c*/ int m_width;
	/*0x20*/ int m_height;
	/*0x24*/ int m_tick;
	/*0x28*/ std::vector<Tab*> m_tabs;
private:
	/*0x2C*/ char pad01[0x18];
public:
	/*0x50*/ int m_tab_count;
	/*0x54*/ Tab* m_active_tab;
	/*0x58*/ Element* m_active_element;
};

class Tab {
public:
	/*0x00*/ const char m_title[0x18];
private:
	/*0x18*/ int one;
public:
	/*0x1C*/ std::vector<Element*> m_elements;
};

class Element {
private:
	/*0x00*/ char pad01[0x24];
public:
	/*0x24*/ Form* m_parent;
	/*0x28*/ const char m_label[0x18];
	/*0x40*/ const char m_file_id[0x18];
private:
	/*0x58*/ char pad02[0x7C];
}; 

class Checkbox : public Element {
public:
	bool enabled;
private:
	char pad01[0x3];
public:
	const char m_label[0x18];
};