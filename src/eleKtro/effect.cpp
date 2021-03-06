/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						"elektroexp.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	EFFECT.CPP					(c)	YoY'01						WEB: www.aestesis.org
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						<stdio.h>
#include						<assert.h>
#include						"effect.h"
#include						"capsule.h"
#include						"elektro.h"
#include						"tcpRemote.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL ACI						Aeffect::CI		= ACI("Aeffect",		GUID(0xE4EC7600,0x00009000), &Anode::CI, 0, NULL);
EDLL ACI						AeffectFront::CI= ACI("AeffectFront",	GUID(0xE4EC7600,0x00009005), &Aobject::CI, 0, NULL);
EDLL ACI						AeffectBack::CI	= ACI("AeffectBack",	GUID(0xE4EC7600,0x00009006), &Aobject::CI, 0, NULL);
EDLL ACI						AeffectInfo::CI	= ACI("AeffectInfo",	GUID(0xE4EC7600,0x00009001), &Aplugz::CI, 0, NULL);
EDLL ACI						AoscNode::CI	= ACI("AoscNode",		GUID(0xE4EC7600,0x00009030), &Anode::CI, 0, NULL);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL char						*efClassName[AeffectInfo::MAXIMUM]={
								"general",
								"in/out",
								"mixer",
								"player",
								"2d effect",
								"3d effect",
								"audio effect" };

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aeffect::Aeffect(QIID qiid, char *name, class AeffectInfo *info, class Acapsule *t) : Anode(name)
{
	capsule=t;
	capsule->effect=this;
	this->qiid=qiid;
	this->info=info;
	onoff=true;
	monoff=true;
	selected=false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aeffect::~Aeffect()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL float Aeffect::getStreamQuality(Apin *pin)
{
	float	q=0.f;
	switch(pin->type&pinDIR)
	{
		case pinIN:
		{
			Atable	*table=getTable();
			ACI		*pci=pin->getCI();
			table->synchronize.enter(__FILE__,__LINE__);
			{
				Apin	*p=table->pins;
				while(p)
				{
					if((p->effect==this)&&((p->type&pinDIR)==pinOUT)&&(p->isCI(pci)))
						q+=p->getStreamQuality();
				}
			}
			table->synchronize.leave();
		}
		break;

		case pinOUT:
		assert(false);
		break;
	}
	return q;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool Aeffect::oscMessage(class AoscMessage *reply, class AoscMessage *msg, class AoscNode *oscnode, int action)
{
	switch(action)
	{
		case oscDOCUMENTATION:
		reply->add(new AoscString(oscnode->help));
		return true;

		case oscTYPESIGNATURE:
		reply->add(new AoscString(oscnode->format));
		return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool Aeffect::NCoscMessage(AoscMessage *reply, AoscMessage *msg, char *path)	// return true, if reply used
{
	Atable	*table=capsule->table;
	if(!strcmp(path, oscSTRING[oscTYPESIGNATURE]))
	{
		reply->add(new AoscString(",")); // return no params
		return true;
	}
	else if(!strcmp(path, oscSTRING[oscDOCUMENTATION]))
	{
		if(this->info->ehelp)
		{
			reply->add(new AoscString(this->info->ehelp));
			return true;
		}
	}
	else if(path[0]=='\\')
	{
		int		len=strlen(path);
		char	*pos=null;
		int		nosc=0;
		for(nosc=oscFIRSTQUERY; nosc<oscLASTQUERY; nosc++)
		{
			pos=strstr(path, oscSTRING[nosc]);
			if(pos)
				break;
		}
		if((nosc>=oscFIRSTQUERY)&&(nosc<oscLASTQUERY))
		{
			if(pos>path)
			{
				char	cname[1024];
				int		i;
				memset(cname, 0, sizeof(cname));
				strncpy(cname, path+1, pos-(path+1));
				{
					char *s=cname;
					while(*s)
					{
						if(*s=='\\')
							*s='/';
						s++;
					}
				}
				for(i=0; i<table->nbControl; i++)
				{
					Acontrol *c=table->control[i];
					if(c->effect==this)
					{
						if(!strcmp(c->name, cname))
							return c->oscMessage(reply, msg, nosc);
					}
				}

				{
					Anode *n=this->lchild;
					while(n)
					{
						if(n->isCI(&AoscNode::CI))
						{
							AoscNode *on=(AoscNode *)n;
							if(!strcmp(on->name, cname))
								return this->oscMessage(reply, msg, on, nosc);
						}
						n=n->prev;
					}
				}

				if(nosc==oscTYPESIGNATURE)	// not a control, it's a group node
				{
					reply->add(new AoscString(","));
					return true;
				}
			}
		}
		else if((len>=1)&&(path[len-1]=='\\'))
		{
			char	cname[1024];
			int		lcname=len-2;
			int		i;

			if(lcname>0)
			{
				strcpy(cname, path+1);
				cname[lcname]=0;
				{
					char *s=cname;
					while(*s)
					{
						if(*s=='\\')
							*s='/';
						s++;
					}
				}
			}

			{
				const int	max=1024;
				char		gstr[max][1024];
				int			nb=0;
				memset(gstr, 0, sizeof(gstr));
				for(i=0; i<table->nbControl; i++)
				{
					Acontrol *c=table->control[i];
					if(c->effect==this)
					{
						if((lcname==-1)||!strncmp(c->name, cname, lcname))
						{
							char	*s=c->name+lcname+1;
							char	*slash=strchr(s, '/');
							int		l=slash?(slash-s):strlen(s);
							bool	exist=false;
							int		j;
							for(j=0; j<nb; j++)
							{
								if(!strncmp(gstr[j], s, l))
								{
									exist=true;
									break;
								}
							}
							if(!exist)
								strncpy(gstr[nb++], s, l);
						}
					}
				}
				{
					Anode *n=this->lchild;
					while(n)
					{
						if(n->isCI(&AoscNode::CI))
						{
							AoscNode *on=(AoscNode *)n;
							if((lcname==-1)||!strncmp(on->name, cname, lcname))
							{
								char	*s=on->name+lcname+1;
								char	*slash=strchr(s, '/');
								int		l=slash?(slash-s):strlen(s);
								bool	exist=false;
								int		j;
								for(j=0; j<nb; j++)
								{
									if(!strncmp(gstr[j], s, l))
									{
										exist=true;
										break;
									}
								}
								if(!exist)
									strncpy(gstr[nb++], s, l);
							}
						}
						n=n->prev;
					}
				}
				for(i=0; i<nb; i++)
					reply->add(new AoscString(gstr[i]));
			}
			return true;
		}
		else
		{
			char	cname[1024];
			int		i;
			strcpy(cname, path+1);
			{
				char *s=cname;
				while(*s)
				{
					if(*s=='\\')
						*s='/';
					s++;
				}
			}
			for(i=0; i<table->nbControl; i++)
			{
				Acontrol *c=table->control[i];
				if(c->effect==this)
				{
					if(!strcmp(c->name, cname))
					{
						return c->oscMessage(reply, msg, oscSETVALUE);
					}
				}
			}

			{
				Anode *n=this->lchild;
				while(n)
				{
					if(n->isCI(&AoscNode::CI))
					{
						AoscNode *on=(AoscNode *)n;
						if(!strcmp(on->name, cname))
							return this->oscMessage(reply, msg, on, oscSETVALUE);
					}
					n=n->prev;
				}
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL int Aeffect::getVideoWidth()
{
	return capsule->table->videoW;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL int Aeffect::getVideoHeight()
{
	return capsule->table->videoH;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool Aeffect::getTableIsRunning()
{
	return capsule->table->running;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL double Aeffect::getBeat()
{
	return capsule->table->beat;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL double Aeffect::getTime()
{
	return capsule->table->getTime();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL int Aeffect::getFrameRate()
{
	return capsule->table->frameRate;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL int Aeffect::getLoopTime()
{
	return capsule->table->vlooptime;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aobject * Aeffect::getFrontLayer()
{
	return capsule->table->front;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aobject * Aeffect::getBackLayer()
{
	return capsule->table->back;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Atable * Aeffect::getTable()
{
	return capsule->table;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL char * Aeffect::getRootPath()
{
	return capsule->table->rootdir;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL int Aeffect::addPresetFile(char *file)
{
	return capsule->table->addPresetFile(file);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool Aeffect::getPresetFile(int n, char *file)
{
	return capsule->table->getPresetFile(n, file);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL Aeffect * Aeffect::getEffect(Aobject *o)
{
	while(o)
	{
		if(o->isCI(&AeffectFront::CI))
			return ((AeffectFront *)o)->effect;
		if(o->isCI(&AeffectBack::CI))
			return ((AeffectBack *)o)->effect;
		if(o->isCI(&Aeffect::CI))
			return (Aeffect*)o;
		o=(Aobject *)o->father;
	}
	return NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AeffectFront::AeffectFront(QIID qiid, char *name, class Aeffect *e, int h) : Aobject(name, e->getFrontLayer(), 10, 0, effectWIDTH, h)
{
	effect=e;
	new Aitem("delete module", "delete this module", context, contextDELEFFECT);
	new Aitem("add module track", "add a module track in the sequencer", context, contextADDTRACK);
	//new Aitem("randomize", "randomize this module", context, contextRANDOMIZE);
	getWindow()->notify(e, nyADDEFFECT);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AeffectFront::~AeffectFront()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool AeffectFront::mouse(int x, int y, int state, int event)
{
	switch(event)
	{
		case mouseLDOWN:
		if((state&mouseSHIFT)||(state&mouseCTRL))
			effect->selected=!effect->selected;
		else
			effect->capsule->select();
		break;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool AeffectFront::notify(Anode *o, int event, dword p)
{
	switch(event)
	{
		case nyCONTEXT:
		{
			int	d=((Aitem *)p)->data;
			switch(d)
			{
				case contextADDTRACK:
				effect->capsule->addTrack();
				return true;
				
				case contextDELEFFECT:
				switch(Amsgbox::msgbox(this, "delete module", "really delete this module ?", Amsgbox::MBYESNO, Amsgbox::MBICONQUESTION))
				{
					case Amsgbox::MBRYES:
					{
						Amenu *m=(Amenu *)o;
						m->toNotify=NULL;	// avoid the nyCLOSE or others messages from menu on a deleted object
					}
					effect->capsule->del();
					break;
				}
				return TRUE;
			}
		}
		break;
	}
	return Anode::notify(o, event, p);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AeffectBack::AeffectBack(QIID qiid, char *name, class Aeffect *e, int h) : Aobject(name, e->getBackLayer(), 10, 0, effectWIDTH, h)
{
	effect=e;
	new Aitem("delete module", "delete this module", context, contextDELEFFECT);
	new Aitem("add module track", "add a module track in the sequencer", context, contextADDTRACK);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AeffectBack::~AeffectBack()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool AeffectBack::mouse(int x, int y, int state, int event)
{
	switch(event)
	{
		case mouseLDOWN:
		if((state&mouseSHIFT)||(state&mouseCTRL))
			effect->selected=!effect->selected;
		else
			effect->capsule->select();
		break;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL bool AeffectBack::notify(Anode *o, int event, dword p)
{
	switch(event)
	{
		case nyCONTEXT:
		{
			int	d=((Aitem *)p)->data;
			switch(d)
			{
				case contextADDTRACK:
				effect->capsule->addTrack();
				return true;
				
				case contextDELEFFECT:
				switch(Amsgbox::msgbox(this, "delete module", "really delete this module ?", Amsgbox::MBYESNO, Amsgbox::MBICONQUESTION))
				{
					case Amsgbox::MBRYES:
					{
						Amenu *m=(Amenu *)o;
						m->toNotify=NULL;	// avoid the nyCLOSE or others messages from menu on a deleted object
					}
					effect->capsule->del();
					break;
				}
				return TRUE;
			}
		}
		break;
	}
	return Anode::notify(o, event, p);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AoscNode::AoscNode(char *name, Anode *container, char *format, char *help) : Anode(name, container)
{
	this->format=strdup(format);
	this->help=strdup(help);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EDLL AoscNode::~AoscNode()
{
	if(format)
		free(format);
	format=null;
	if(help)
		free(help);
	help=null;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

