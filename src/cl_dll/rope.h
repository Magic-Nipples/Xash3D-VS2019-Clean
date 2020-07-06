/*
AR Software (c) 2004-2007. ArrangeMent, S2P Physics, Spirit Of Half-Life and their
logos are the property of their respective owners.

Edits by: Magic_Nipples, 2019.
*/
//magic nipples - ropes

#define MAX_ROPES		128

//for Bézier calcs
#define	B1(t)			(t*t)
#define	B2(t)			(2*t*(1-t))
#define	B3(t)			((1-t)*(1-t))

#define START			0
#define MID_POINT		1
#define END				2
#define MAX_SEGMENTS	64

struct sRope
{
	vec3_t	mySpline[MAX_SEGMENTS];
	vec3_t myPoints[3];

	int	mySegments;
	int	myLenght;
	float	myScale;

	int	r;
	int	g;
	int	b;

	char mySpriteFile[512];

	bool bDraw;
	bool bCheckLight;
};

class GLRopeRender
{
public:
	GLRopeRender();
	~GLRopeRender();

	void DrawRopes(float fltime);
	void ResetRopes();

	void CreateRope(char* datafile, vec3_t start_source, vec3_t end_source);

	void DrawBeam(vec3_t start, vec3_t end, float width, char* Sprite, bool bCheckLight, int r, int g, int b);

	sRope hRope[MAX_ROPES];

private:
	int rope_id;
};

extern GLRopeRender gRopeRender;