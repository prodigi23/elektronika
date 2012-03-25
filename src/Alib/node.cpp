/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						"export.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//	NODE.CPP					(c)	YoY'99						WEB: search aestesis
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include						<assert.h>
#include						<stdio.h>
#include						"node.h"
#include						"window.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL char *						Alasterror;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL ACI						*aciList=NULL;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL AdbgNode					*AdbgNode::nodes=	NULL;
ADLL int						AdbgNode::nbnodes=	0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL AdbgNode::AdbgNode(Anode *n)
{
	node=n;
	next=nodes;
	nodes=this;
	nbnodes++;
}

ADLL AdbgNode::~AdbgNode()
{
	nbnodes--;
}

ADLL void AdbgNode::add(Anode *n)
{
	new AdbgNode(n);
}

ADLL void AdbgNode::del(Anode *n)
{
	AdbgNode	*dn=nodes;
	AdbgNode	*o=NULL;
	while(dn)
	{
		if(dn->node==n)
		{
			if(o)
				o->next=dn->next;
			else
				nodes=dn->next;
			delete(dn);
			return;
		}
		o=dn;
		dn=dn->next;
	}
	assert(false);
}

ADLL void AdbgNode::check()
{
	AdbgNode	*dn=nodes;
	char		str[1024];
	sprintf(str, "[Alib.Anode: %d]\n", nbnodes);
	OutputDebugString(str);
	while(dn)
	{
		Anode	*n=dn->node;
		ACI		*ci=n->getCI();
		sprintf(str, "adr[%8x] node[%s] name[%s]\n", (int)n,  ci->name, n->name);
		OutputDebugString(str);
		dn=dn->next;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL ACI::ACI(char *name, qword guid, ACI *inherited, int np, class Aproperties *p)
{
	assert(guid!=GUID(0,0));

	this->name=name;
	this->guid=guid;
	this->inherited=inherited;
	this->nproperties=np;
	this->properties=p;

#ifdef _DEBUG
	{
		ACI	*a=inherited;
		while(a)
		{
			assert(a->guid!=guid);	// no end loop (fatherCI==objectCI)
			a=a->inherited;
		}
	}
	{
		ACI	*a=aciList;
		while(a)
		{
			assert(a->guid!=guid);	// guid used before
			a=a->next;
		}
	}
#endif
	next=aciList;
	aciList=this;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL ACI::~ACI()
{
	ACI	*a=aciList;
	ACI	*o=NULL;
	while(a)
	{
		if(a==this)
		{
			if(o)
				o->next=next;
			else
				aciList=next;
			return;
		}
		o=a;
		a=a->next;
	}	
	assert(FALSE);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//static Aproperties				properties[]={guidSTRINGP, (int)&(((Anode *)NULL)->name), "name", "object instance name" };
										
ADLL ACI						Anode::CI=ACI("Anode", GUID(0xAE57E515, 0x00000001), NULL, 0, NULL);

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL Anode::Anode(char *name, Anode *father)
{
	#ifdef _DEBUG
	AdbgNode::add(this);
	#endif
	
	if(name&&name[0])
		this->name=strdup(name);
	else
		this->name=NULL;
	this->father=NULL;
	next=NULL;
	prev=NULL;
	fchild=NULL;
	lchild=NULL;
	state=stateENABLE;
	if(father)
		father->add(this);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL Anode::~Anode()
{
	#ifdef _DEBUG
	AdbgNode::del(this);
	#endif
	
	if(name)
		free(name);
	if(father)
		father->del(this);
	while(fchild)
		delete(fchild);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL bool Anode::is(char *name)
{
	ACI		*sci=getCI();
	while(sci)
	{
		if(!strcmp(sci->name, name))
			return TRUE;
		sci=sci->inherited;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL bool Anode::isCI(ACI *ci)
{
	ACI		*sci=getCI();
	while(sci)
	{
		if(sci->guid==ci->guid)
			return TRUE;
		sci=sci->inherited;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL bool Anode::isGUID(qword guid)
{
	ACI		*sci=getCI();
	while(sci)
	{
		if(sci->guid==guid)
			return TRUE;
		sci=sci->inherited;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::setName(char *name)
{
	if(this->name)
		free(this->name);
	if(name&&name[0])
		this->name=strdup(name);
	else
		this->name=NULL;
		
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::clear()
{
	while(fchild)
		delete(fchild);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::add(Anode *o)
{
	Anode	*os=NULL;
	Anode	*s=fchild;
	assert((o->father==NULL)||(o->father==this));
	o->father=this;
	while(s)
	{
		if(!(s->state&stateTOP))
			break;
		os=s;
		s=s->next;
	}
	if(os)
	{
		if(os->next)
		{
			o->next=os->next;
			os->next->prev=o;
			os->next=o;
			o->prev=os;
		}
		else
		{
			o->prev=os;
			o->next=NULL;
			os->next=o;
			lchild=o;
		}
	}
	else
	{
		if(fchild)
		{
			o->next=fchild;
			o->prev=NULL;
			fchild->prev=o;
			fchild=o;
		}
		else
		{
			o->prev=NULL;
			o->next=NULL;
			fchild=lchild=o;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::del(Anode *o)
{
	if(o->prev)
		o->prev->next=o->next;
	else
		fchild=o->next;
	if(o->next)
		o->next->prev=o->prev;
	else
		lchild=o->prev;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL Anode * Anode::getChild(ACI *ci)
{
	Anode	*s=fchild;
	while(s)
	{
		if(s->isCI(ci))
			return s;
		s=s->next;
	}
	return null;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL int Anode::count()
{
	Anode	*i=fchild;
	int		n=0;
	while(i)
	{
		i=i->next;
		n++;
	}
	return n;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::sort(int flags)
{
	if(fchild)
	{
		bool	b=true;
		while(b)
		{
			Anode	*i=fchild;
			b=false;
			while(i)
			{
				Anode	*n=i->next;
				Anode	*oi=i->prev;
				if(oi)
				{
					int		n=0;
					if(i->name&&oi->name)
						n=stricmp(i->name, oi->name);
					bool	revers=(flags&sortDEC)?(n>0):(n<0);
					if(revers)
					{
						Anode	*pi=oi->prev;
						Anode	*ni=i->next;
						if(pi)
							pi->next=i;
						else
							this->fchild=i;
						if(ni)
							ni->prev=oi;
						else
							this->lchild=oi;
						i->prev=pi;
						i->next=oi;
						oi->prev=i;
						oi->next=ni;
						b=true;
					}
				}
				i=n;
			}
		}
	}
	if(flags&sortRECURSIVE)
	{
		Anode	*i=fchild;
		while(i)
		{
			i->sort(flags);
			i=i->next;				
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ADLL void Anode::asyncNotify(Anode *o, int event, dword p)
{
	Awindow::NCasyncNotify(new asyncMessage(this, o, event, p));
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////