#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

static char *pgm_name;

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

int main(int argc, char *argv[])
{
    pgm_name = argv[0];
    if (argc > 1)
    {
        vertex *vertices;
        //Set vertices array to maximum possible size
        vertices = malloc(sizeof(vertex) * ((argc - 1) * 2));
        if (vertices == NULL)
        {
            fprintf(stderr, "malloc for vertices failed.");
            exit(EXIT_FAILURE);
        }
        int numberOfVertices = 0;
        edge edges[argc - 1];

        for (int i = 1; i < argc; i++)
        {
            long int v1;
            char *nptr = argv[i];
            char *endptr;
            v1 = strtol(nptr, &endptr, 10);

            if (v1 == LONG_MIN || v1 == LONG_MAX)
            {
                fprintf(stderr, "%s: input could not be parsed.The identifier before the '-'sign of at least one edge is too big to be computed. %s\n", pgm_name, strerror(errno));
                exit(EXIT_FAILURE);
            }

            if (endptr != NULL)
            {
                if (endptr == nptr || endptr[0] != '-')
                {
                    fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed before the '-' sign.\n", pgm_name);
                    exit(EXIT_FAILURE);
                }
            }

            long v2;
            char *endptr2;
            v2 = strtol(endptr + 1, &endptr2, 10);

            if (v2 == LONG_MIN || v2 == LONG_MAX)
            {
                fprintf(stderr, "%s: input could not be parsed. The identifier after the '-' sign of at least one edge is too big to be computed. %s\n", pgm_name, strerror(errno));
                exit(EXIT_FAILURE);
            }

            if (endptr != NULL)
            {
                if (endptr == nptr)
                {
                    fprintf(stderr, "%s: input could not be parsed. At least one edge is malformed after the '-' sign.\n", pgm_name);
                    exit(EXIT_FAILURE);
                }
            }

            vertex vert1 = {v1, EMTPY};
            vertex vert2 = {v2, EMTPY};

            //Set first vertex.
            if (numberOfVertices == 0)
            {
                vertices[0] = vert1;
                numberOfVertices++;
            }

            //Check for each vertex if it already exists in the array. Add if it does not.
            int vert1InArray = 0;
            int vert2InArray = 0;

            for (int l = 0; l < numberOfVertices; l++)
            {
                if (vertices[l].id == vert1.id)
                {
                    vert1InArray = 1;
                }

                if (vertices[l].id == vert2.id)
                {
                    vert2InArray = 1;
                }
            }

            if (vert1InArray == 0)
            {
                vertices[numberOfVertices] = vert1;
                numberOfVertices++;
            }
            if (vert2InArray == 0)
            {
                vertices[numberOfVertices] = vert2;
                numberOfVertices++;
            }

            //Add to edges
            edge e = {vert1, vert2};
            edges[i - 1] = e;
        }

        //Set vertices to exact size
        if (realloc(vertices, sizeof(vertex) * numberOfVertices) == NULL)
        {
            fprintf(stderr, "realloc for vertices failed.");
            exit(EXIT_FAILURE);
        }

        for (int m = 0; m < numberOfVertices; m++)
        {
            printf("Vertex: %ld\n", vertices[m].id);
        }

        for (int k = 0; k < argc - 1; k++)
        {
            printf("Edge: %ld-%ld\n", edges[k].v1.id, edges[k].v2.id);
        }
    }
    else
    {
        fprintf(stderr, "%s: needs at least one edge as input.\n", pgm_name);
        exit(EXIT_FAILURE);
    }
}