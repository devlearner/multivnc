

#include "VNCCanvas.h"
#include "res/vnccursor.xbm"
#include "res/vnccursor-mask.xbm"

BEGIN_EVENT_TABLE(VNCCanvas, wxScrolledWindow)
    EVT_PAINT  (VNCCanvas::onPaint)
    EVT_MOUSE_EVENTS (VNCCanvas::onMouseAction)
    EVT_KEY_DOWN (VNCCanvas::onKeyDown)
    EVT_KEY_UP (VNCCanvas::onKeyUp)
    EVT_CHAR (VNCCanvas::onChar)
END_EVENT_TABLE();

#define VNCCANVAS_SCROLL_RATE 10



/*
  constructor/destructor
*/

VNCCanvas::VNCCanvas(wxWindow* parent, VNCConn* c):
  wxScrolledWindow(parent, -1, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL | wxWANTS_CHARS)
{
  conn = c;
  framebuffer_rect.x = 0;
  framebuffer_rect.y = 0;
  adjustSize(); // this sets width and height of both the rect and the canvas according to conn
 
  SetScrollRate(VNCCANVAS_SCROLL_RATE, VNCCANVAS_SCROLL_RATE);
 
  // this kinda cursor creation works everywhere
  wxBitmap vnccursor_bitmap(vnccursor_bits, 16, 16);
  wxBitmap vnccursor_mask_bitmap(vnccursor_mask, 16, 16);
  vnccursor_bitmap.SetMask(new wxMask(vnccursor_mask_bitmap));
  wxImage vnccursor_image = vnccursor_bitmap.ConvertToImage();
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, 8);
  vnccursor_image.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, 8);
  SetCursor(wxCursor(vnccursor_image));
}



/*
  private members
*/

void VNCCanvas::onPaint(wxPaintEvent &WXUNUSED(event))
{
  wxPaintDC dc(this);

  // find out where the window is scrolled to
  static int rx, ry;                    
  GetViewStart(&rx, &ry);
  rx *= VNCCANVAS_SCROLL_RATE;
  ry *= VNCCANVAS_SCROLL_RATE;

  
  // get the update rect list
  wxRegionIterator upd(GetUpdateRegion()); 
  while(upd)
    {
      wxRect viewport_rect(upd.GetRect());
     
      wxLogDebug(wxT("VNCCanvas %p: got repaint event: on screen (%i,%i,%i,%i)"),
		 this,
		 viewport_rect.x,
		 viewport_rect.y,
		 viewport_rect.width,
		 viewport_rect.height);

      wxRect fromframebuffer_rect = viewport_rect;
      fromframebuffer_rect.x += rx;
      fromframebuffer_rect.y += ry;
      fromframebuffer_rect.Intersect(framebuffer_rect);
      
      wxLogDebug(wxT("VNCCanvas %p: got repaint event: in framebuffer (%i,%i,%i,%i)"),
		 this,
		 fromframebuffer_rect.x,
		 fromframebuffer_rect.y,
		 fromframebuffer_rect.width,
		 fromframebuffer_rect.height);
      
      if(! fromframebuffer_rect.IsEmpty())
	{
      	  wxBitmap region = conn->getFrameBufferRegion(fromframebuffer_rect);
	  dc.DrawBitmap(region, viewport_rect.x, viewport_rect.y);
	}

      ++upd;
    }
}






void VNCCanvas::onMouseAction(wxMouseEvent &event)
{
  if(event.Entering())
    SetFocus();

  wxClientDC dc(this);
  PrepareDC(dc);

  event.m_x = dc.DeviceToLogicalX(event.m_x);
  event.m_y = dc.DeviceToLogicalY(event.m_y);

  conn->sendPointerEvent(event);
}


void VNCCanvas::onKeyDown(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, true, false);
}


void VNCCanvas::onKeyUp(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, false, false);
}


void VNCCanvas::onChar(wxKeyEvent &event)
{
  conn->sendKeyEvent(event, true, true);
}




/*
  public members
*/

void VNCCanvas::drawRegion(wxRect& rect)
{
  wxLogDebug(wxT("VNCCanvas %p: drawing region (%i,%i,%i,%i)"),
	     this,
	     rect.x,
	     rect.y,
	     rect.width,
	     rect.height);

  wxClientDC dc(this);
  DoPrepareDC(dc); // this adjusts coordinates when window is scrolled
  wxBitmap region = conn->getFrameBufferRegion(rect);
  dc.DrawBitmap(region, rect.x, rect.y);
}



void VNCCanvas::adjustSize()
{
  wxLogDebug(wxT("VNCCanvas %p: adjusting size to (%i, %i)"),
	     this,
	     conn->getFrameBufferWidth(),
	     conn->getFrameBufferHeight());

  ClearBackground();
  Refresh();

  framebuffer_rect.width = conn->getFrameBufferWidth();
  framebuffer_rect.height = conn->getFrameBufferHeight();

  SetSize(framebuffer_rect.width, framebuffer_rect.height);
  SetVirtualSize(framebuffer_rect.width, framebuffer_rect.height);
}
