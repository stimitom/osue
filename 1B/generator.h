enum colour
{
    EMTPY,
    RED,
    GREEN,
    BLUE
};

typedef struct vert
{
    long int id;
    enum colour colour;
} vertex;

typedef struct edg
{
    vertex v1;
    vertex v2;
} edge;

typedef struct sol {
    edge *edges;
} solution;