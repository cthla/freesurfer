// generated by Fast Light User Interface Designer (fluid) version 1.0110

#include "kvlImageViewingConsoleGUI.h"

void kvlImageViewingConsoleGUI::cb_m_Window_i(Fl_Double_Window*, void*) {
  exit( 0 );
}
void kvlImageViewingConsoleGUI::cb_m_Window(Fl_Double_Window* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->user_data()))->cb_m_Window_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ImageViewer_i(kvl::ImageViewer*, void*) {
  //this->SelectTriangleContainingPoint( Fl::event_x(), m_ImageViewer->h() - Fl::event_y() );
}
void kvlImageViewingConsoleGUI::cb_m_ImageViewer(kvl::ImageViewer* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_m_ImageViewer_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_OverlayOpacity_i(Fl_Value_Slider*, void*) {
  this->SetOverlayOpacity( m_OverlayOpacity->value() );
}
void kvlImageViewingConsoleGUI::cb_m_OverlayOpacity(Fl_Value_Slider* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_m_OverlayOpacity_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_CoronalSliceNumber_i(Fl_Value_Slider*, void*) {
  this->SetSliceLocation( 
  static_cast< unsigned int >( m_SagittalSliceNumber->value() ),
  static_cast< unsigned int >( m_CoronalSliceNumber->value() ),
  static_cast< unsigned int >( m_AxialSliceNumber->value() ) );
}
void kvlImageViewingConsoleGUI::cb_m_CoronalSliceNumber(Fl_Value_Slider* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_m_CoronalSliceNumber_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_SagittalSliceNumber_i(Fl_Value_Slider*, void*) {
  m_CoronalSliceNumber->do_callback();
}
void kvlImageViewingConsoleGUI::cb_m_SagittalSliceNumber(Fl_Value_Slider* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_m_SagittalSliceNumber_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_AxialSliceNumber_i(Fl_Value_Slider*, void*) {
  m_CoronalSliceNumber->do_callback();
}
void kvlImageViewingConsoleGUI::cb_m_AxialSliceNumber(Fl_Value_Slider* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_m_AxialSliceNumber_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ViewOne_i(Fl_Round_Button*, void*) {
  this->ShowSelectedView();
}
void kvlImageViewingConsoleGUI::cb_m_ViewOne(Fl_Round_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->parent()->user_data()))->cb_m_ViewOne_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ViewTwo_i(Fl_Round_Button*, void*) {
  this->ShowSelectedView();
}
void kvlImageViewingConsoleGUI::cb_m_ViewTwo(Fl_Round_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->parent()->user_data()))->cb_m_ViewTwo_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ViewFour_i(Fl_Round_Button*, void*) {
  this->ShowSelectedView();
}
void kvlImageViewingConsoleGUI::cb_m_ViewFour(Fl_Round_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->parent()->user_data()))->cb_m_ViewFour_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ViewThree_i(Fl_Round_Button*, void*) {
  this->ShowSelectedView();
}
void kvlImageViewingConsoleGUI::cb_m_ViewThree(Fl_Round_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->parent()->user_data()))->cb_m_ViewThree_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_m_ViewFive_i(Fl_Round_Button*, void*) {
  this->ShowSelectedView();
}
void kvlImageViewingConsoleGUI::cb_m_ViewFive(Fl_Round_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->parent()->user_data()))->cb_m_ViewFive_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_Screen_i(Fl_Button*, void*) {
  this->GetScreenShotSeries( 0 );
}
void kvlImageViewingConsoleGUI::cb_Screen(Fl_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_Screen_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_Screen1_i(Fl_Button*, void*) {
  this->GetScreenShotSeries( 1 );
}
void kvlImageViewingConsoleGUI::cb_Screen1(Fl_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_Screen1_i(o,v);
}

void kvlImageViewingConsoleGUI::cb_Screen2_i(Fl_Button*, void*) {
  this->GetScreenShotSeries( 2 );
}
void kvlImageViewingConsoleGUI::cb_Screen2(Fl_Button* o, void* v) {
  ((kvlImageViewingConsoleGUI*)(o->parent()->user_data()))->cb_Screen2_i(o,v);
}

kvlImageViewingConsoleGUI::kvlImageViewingConsoleGUI() {
  { m_Window = new Fl_Double_Window(1280, 1030, "kvlImageViewingConsole");
    m_Window->callback((Fl_Callback*)cb_m_Window, (void*)(this));
    { m_ImageViewer = new kvl::ImageViewer(10, 10, 1000, 1000);
      m_ImageViewer->box(FL_FLAT_BOX);
      m_ImageViewer->color((Fl_Color)FL_FOREGROUND_COLOR);
      m_ImageViewer->selection_color((Fl_Color)FL_BACKGROUND_COLOR);
      m_ImageViewer->labeltype(FL_NORMAL_LABEL);
      m_ImageViewer->labelfont(0);
      m_ImageViewer->labelsize(14);
      m_ImageViewer->labelcolor((Fl_Color)FL_FOREGROUND_COLOR);
      m_ImageViewer->callback((Fl_Callback*)cb_m_ImageViewer);
      m_ImageViewer->align(FL_ALIGN_TOP);
      m_ImageViewer->when(FL_WHEN_RELEASE);
      m_ImageViewer->end();
    } // kvl::ImageViewer* m_ImageViewer
    { m_OverlayOpacity = new Fl_Value_Slider(1070, 40, 175, 25, "Overlay opacity:");
      m_OverlayOpacity->type(1);
      m_OverlayOpacity->textsize(14);
      m_OverlayOpacity->callback((Fl_Callback*)cb_m_OverlayOpacity);
      m_OverlayOpacity->align(FL_ALIGN_TOP);
    } // Fl_Value_Slider* m_OverlayOpacity
    { m_CoronalSliceNumber = new Fl_Value_Slider(1075, 245, 175, 25, "Coronal slice number:");
      m_CoronalSliceNumber->type(1);
      m_CoronalSliceNumber->step(1);
      m_CoronalSliceNumber->textsize(14);
      m_CoronalSliceNumber->callback((Fl_Callback*)cb_m_CoronalSliceNumber);
      m_CoronalSliceNumber->align(FL_ALIGN_TOP);
    } // Fl_Value_Slider* m_CoronalSliceNumber
    { m_SagittalSliceNumber = new Fl_Value_Slider(1075, 340, 175, 25, "Sagittal slice number:");
      m_SagittalSliceNumber->type(1);
      m_SagittalSliceNumber->step(1);
      m_SagittalSliceNumber->textsize(14);
      m_SagittalSliceNumber->callback((Fl_Callback*)cb_m_SagittalSliceNumber);
      m_SagittalSliceNumber->align(FL_ALIGN_TOP);
    } // Fl_Value_Slider* m_SagittalSliceNumber
    { m_AxialSliceNumber = new Fl_Value_Slider(1075, 431, 175, 25, "Axial slice number:");
      m_AxialSliceNumber->type(1);
      m_AxialSliceNumber->step(1);
      m_AxialSliceNumber->textsize(14);
      m_AxialSliceNumber->callback((Fl_Callback*)cb_m_AxialSliceNumber);
      m_AxialSliceNumber->align(FL_ALIGN_TOP);
    } // Fl_Value_Slider* m_AxialSliceNumber
    { Fl_Group* o = new Fl_Group(1104, 554, 124, 109, "View");
      o->box(FL_DOWN_BOX);
      { m_ViewOne = new Fl_Round_Button(1114, 561, 29, 29);
        m_ViewOne->type(102);
        m_ViewOne->down_box(FL_ROUND_DOWN_BOX);
        m_ViewOne->callback((Fl_Callback*)cb_m_ViewOne);
        m_ViewOne->align(FL_ALIGN_CENTER|FL_ALIGN_INSIDE);
      } // Fl_Round_Button* m_ViewOne
      { m_ViewTwo = new Fl_Round_Button(1194, 560, 27, 28);
        m_ViewTwo->type(102);
        m_ViewTwo->down_box(FL_ROUND_DOWN_BOX);
        m_ViewTwo->callback((Fl_Callback*)cb_m_ViewTwo);
      } // Fl_Round_Button* m_ViewTwo
      { m_ViewFour = new Fl_Round_Button(1194, 626, 25, 29);
        m_ViewFour->type(102);
        m_ViewFour->down_box(FL_ROUND_DOWN_BOX);
        m_ViewFour->callback((Fl_Callback*)cb_m_ViewFour);
      } // Fl_Round_Button* m_ViewFour
      { m_ViewThree = new Fl_Round_Button(1114, 629, 24, 21);
        m_ViewThree->type(102);
        m_ViewThree->down_box(FL_ROUND_DOWN_BOX);
        m_ViewThree->callback((Fl_Callback*)cb_m_ViewThree);
      } // Fl_Round_Button* m_ViewThree
      { m_ViewFive = new Fl_Round_Button(1149, 593, 30, 27);
        m_ViewFive->type(102);
        m_ViewFive->down_box(FL_ROUND_DOWN_BOX);
        m_ViewFive->value(1);
        m_ViewFive->callback((Fl_Callback*)cb_m_ViewFive);
      } // Fl_Round_Button* m_ViewFive
      o->end();
    } // Fl_Group* o
    { Fl_Button* o = new Fl_Button(1095, 370, 135, 25, "Screen shot series");
      o->callback((Fl_Callback*)cb_Screen);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(1095, 275, 135, 25, "Screen shot series");
      o->callback((Fl_Callback*)cb_Screen1);
    } // Fl_Button* o
    { Fl_Button* o = new Fl_Button(1095, 460, 135, 25, "Screen shot series");
      o->callback((Fl_Callback*)cb_Screen2);
    } // Fl_Button* o
    m_Window->end();
  } // Fl_Double_Window* m_Window
}

kvlImageViewingConsoleGUI::~kvlImageViewingConsoleGUI() {
}

void kvlImageViewingConsoleGUI::SetOverlayOpacity( float overlayOpacity ) {
}

void kvlImageViewingConsoleGUI::SetSliceLocation( unsigned int, unsigned int, unsigned int ) {
}

void kvlImageViewingConsoleGUI::ShowSelectedView() {
}

void kvlImageViewingConsoleGUI::GetScreenShot() {
}

void kvlImageViewingConsoleGUI::GetScreenShotSeries( int directionNumber ) {
}
