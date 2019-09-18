typedef enum {
	SP_RECT,
	SP_CIRC,
	SP_LINE,
	SP_POLYL,
	SP_POLYG,
} SPType;

struct sShape{
	SPType type;
};

typedef struct sShape Shape;
typedef struct sRect Rect;
typedef struct sShape Shape;


struct sRect{
	Shape shape;
	int x;
	int y;
	int width;
	int height;
};

struct sCircle{
	Shape shape;
	int cx;
	int cy;
	int r;
};
