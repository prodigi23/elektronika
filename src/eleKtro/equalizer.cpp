/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						"elektroexp.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	EQUALIZER.CPP				(c)	YoY'02						WEB: www.aestesis.org
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						<windows.h>
#include						<stdio.h>
#include						<math.h>
#include						<assert.h>

#include						"equalizer.h"
#include						"interface.h"
#include						"../alib/resource.h"
#include						"effect.h"
#include						"resource.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL ACI						Aequalizer::CI=ACI("Aequalizer", GUID(0xE4EC7600,0x00010110), &AcontrolObj::CI, 0, NULL);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Aspline
{
public:
	int							type;

	enum
	{
		BSPLINE,
		BEZIER
	};

								Aspline				(int nbpoints, int type=BSPLINE);
	virtual						~Aspline			();

	Apoint						eval				(int i, float t);

	Apoint						*pts;
	int							nbpts;

private:

	float						bspline				(int i, float t);
	float						bezier				(int i, float t);					
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Aspline::Aspline(int nbpoints, int type)
{
	this->type=type;
	pts=new Apoint[nbpoints];
	nbpts=nbpoints;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Aspline::~Aspline()
{
	delete(pts);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float Aspline::bspline(int i, float t)
{
	switch(i)
	{
		case -2:
		return (((-t+3.f)*t-3.f)*t+1.f)/6.f;
		case -1:
		return (((3.f*t-6.f)*t)*t+4.f)/6.f;
		case 0:
		return (((-3.f*t+3.f)*t+3.f)*t+1.f)/6.f;
		case 1:
		return (t*t*t)/6.f;
    }
    return 0.f;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

float Aspline::bezier(int i, float t)
{
	switch (i)
	{
		case 0:
		return (1.f-t)*(1.f-t)*(1.f-t);
		case 1:
		return 3.f*t*(1.f-t)*(1.f-t);
		case 2:
		return 3.f*t*t*(1.f-t);
		case 3:
		return t*t*t;
	}
	return 0.f;
}
  
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Apoint Aspline::eval(int i, float t)
{
	float	px=0;
	float	py=0;
	int		j;

	switch(type)
	{
		case BSPLINE:
		for(j=-2; j<=1; j++)
		{
			int		n=mini(maxi(i+j, 0), nbpts-1);
			float	v=bspline(j, t);
			px += v * pts[i+j].x;
			py += v * pts[i+j].y;
		}
		break;

		case BEZIER:
		for(j=0; j<=3; j++)
		{
			int		n=maxi(mini(i+j, nbpts-1), 0);
			float	v=bezier(j, t);
			px += v * pts[n].x;
			py += v * pts[n].y;
		}
		break;
	}
	return Apoint((int)px, (int)py);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static dword	colors[]={ 0xff800000, 0xff804000, 0xff808000, 0xff408000, 0xff008000, 0xff008040, 0xff008080, 0xff004080, 0xff000080 };

EDLL Aequalizer::Aequalizer(QIID qiid, char *name, Aobject *L, int x, int y, int w, int h, int nband, int type) : AcontrolObj(name, L, x, y, w, h)
{
	int	i;
	assert(nband<=MAXBANDS);
	this->nband=nband;
	this->type=type;
	modify=false;
	test=false;
	control=new Acontrol(qiid, name, Aeffect::getEffect(L), this, Acontrol::CONTROLER_01+nband-1);
	assert(nband<countof(colors));
	for(i=0; i<nband; i++)
	{
		char	name[128];
		sprintf(name, "band %d", i);
		control->setInfo(Acontrol::CONTROLER_01+i, name, colors[i]);
		vband[i]=0.5f;
	}
	bgcolor=0xff808080;
	linecolor=0xffffffff;
	barcolorb=0xffa0a0a0;
	barcolor=0xff000000;
	spline=new Aspline(nband+6); // , Aspline::BEZIER
	timer(CTRLTIMEREPAINT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aequalizer::~Aequalizer()
{
	delete(control);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool Aequalizer::mouse(int x, int y, int state, int event)
{
	switch(event)
	{
		case mouseLDOWN:
		test=true;
		mouseCapture(true);
		control->select();
		return TRUE;

		case mouseLUP:
		if(test)
		{
			mouseCapture(false);
			test=false;
		}
		return TRUE;

		case mouseNORMAL:
		cursor(cursorHANDSEL);
		if(test)
		{
			int	wb=(pos.w-8)/nband;
			int	n=maxi(mini((x-5)/wb, nband-1), 0);
			int	h=(int)(pos.h-6);
			int	yy=mini(maxi((pos.h-4)-y, 0), h);
			set(n, (float)yy/(float)h);
			repaint();
			father->notify(this, nyCHANGE, n);
		}
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL void Aequalizer::set(int band, float value)
{
	vband[band]=value;
	control->set(Acontrol::CONTROLER_01+band, value);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL float Aequalizer::get(int band)
{
	assert((band>=0)&&(band<nband));
	return vband[band];
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool Aequalizer::notify(Anode *o, int event, dword p)
{
	if(o==control)
	{
		int	control=p>>16;
		int	value=p&0xffff;
		int	n=control-Acontrol::CONTROLER_01;
		vband[n]=(float)value/127.f;
		modify=true;
		father->notify(this, nyCHANGE, n);
		return true;
	}
	return Aobject::notify(o, event, p);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL void Aequalizer::pulse()
{
	if(modify)
	{
		repaint();
		modify=false;
	}	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL void Aequalizer::paint(Abitmap *b)
{
	b->box(0, 0, pos.w-1, pos.h-1, 0xff5A5956);
	b->box(1, 1, pos.w-2, pos.h-2, 0xff5A5956);
	b->boxf(2, 2, pos.w-3, pos.h-3, bgcolor);
	{
		int	wb=(pos.w-8)/nband;
		int	wbm=wb>>1;
		int	wt=wb-3;
		int	i;
		int	xb=5;
		int	xl=5;
		if(type&VIEWBAR)
		{
			for(i=0; i<nband; i++)
			{
				int	hh=pos.h-6;
				int	h=(int)(vband[i]*(pos.h-6));
				int	y=pos.h-4;
				int	j;
				for(j=0; j<h; j++)
				{
					if((j&3)!=3)
						b->line(xb, y, xb+wt, y, barcolor);
					y--;
				}
				for(j=h; j<hh; j++)
				{
					if((j&3)!=3)
						b->line(xb, y, xb+wt, y, barcolorb);
					y--;
				}
				xb+=wb;
			}
		}
		if(type&VIEWSPLINE)
		{
			for(i=0; i<nband; i++)
			{
				int		j;
				float	t=0.f;
				float	dt=1.f/(float)wb;
				for(j=0; j<wb; j++)
				{
					Apoint	p=spline->eval(i+3, t);
					b->pixel(p.x, p.y, linecolor);
					t+=dt;
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////