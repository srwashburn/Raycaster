#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

int line = 1;

int width;
int height;

double cam_width;
double cam_height;

typedef struct {
  int kind; // 0 = plane, 1 = sphere, 2 = camera;
  double color[3];
  //char sym;
  union {
    struct {
      double position[3]; //center position;
      double normal[3];
    } plane;
    struct {
      double position[3]; //center position;
      double radius;
    } sphere;
    struct{
        //ignore object
    } camera;
  };
} Object;





typedef struct Pixel {

    unsigned char r, g, b;

 }   Pixel;



typedef struct {
    double width;
    double height;

} Camera;



// next_c() wraps the getc() function and provides error checking and line
// number maintenance
int next_c(FILE* json) {
  int c = fgetc(json);
#ifdef DEBUG
  printf("next_c: '%c'\n", c);
#endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
    ;
  }
  return c;
}


// expect_c() checks that the next character is d.  If it is not it emits
// an error.
void expect_c(FILE* json, int d) {
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);
}


// skip_ws() skips white space in the file.
void skip_ws(FILE* json) {
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}


// next_string() gets the next string from the file handle and emits an error
// if a string can not be obtained.
char* next_string(FILE* json) {
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}

double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Object** read_scene(char* filename, Object** objects) {

    int c;
  FILE* json = fopen(filename, "r");

  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }

  skip_ws(json);

  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects
  int index = 0;
  while (1) {
    //printf("Index: %d \n", index);
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      exit(1);
    }
    if (c == '{') {
      skip_ws(json);

      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
	fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
	exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) {
            if(index ==0 ){
                index = -1;
            }else{
                index--;
            }
      } else if (strcmp(value, "sphere") == 0) {
          objects[index] = malloc(sizeof(Object));
          objects[index]->kind = (int) 1;
          //printf("Kind: %d \n", objects[index]->kind);
      } else if (strcmp(value, "plane") == 0) {
          objects[index] = malloc(sizeof(Object));
          objects[index]->kind = (int) 0;
          //printf("Kind: %d \n", objects[index]->kind);
      } else {
	fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
	exit(1);
      }

      skip_ws(json);

      while (1) {
	// , }
	c = next_c(json);
	//printf("C at front of loop: %c\n", c);
	if (c == '}') {
	  // stop parsing this object
	  break;
	} else if (c == ',') {
	  // read another field
	  skip_ws(json);
	  char* key = next_string(json);
	  //printf(key);
	  skip_ws(json);
	  expect_c(json, ':');
	  skip_ws(json);
	  if ((strcmp(key, "width") == 0) ||
	      (strcmp(key, "height") == 0) ||
	      (strcmp(key, "radius") == 0)) {
	    double value = next_number(json);
	    //printf("Value: %f\n", value);
	    if(strcmp(key, "width") == 0){
        cam_width = value;
	  }
	  if(strcmp(key, "height") == 0){
        cam_height = value;
	  }
	  if(strcmp(key, "radius") == 0){
        if(objects[index]->kind == 1){
            objects[index]->sphere.radius = value;
            //printf("Radius: %f\n", objects[index]->sphere.radius);
            //printf("got to radius...");
        }
	  }
	  } else if ((strcmp(key, "color") == 0) ||
		     (strcmp(key, "position") == 0) ||
		     (strcmp(key, "normal") == 0)) {
	    double* value = next_vector(json);
	    if(strcmp(key, "color") == 0){
            objects[index]->color[0] = value[0];
            objects[index]->color[1] = value[1];
            objects[index]->color[2] = value[2];
            //printf("got to color...");
         }
         else if(strcmp(key, "position") == 0){
            if(objects[index]->kind == 0){
                objects[index]->plane.position[0] = value[0];
                objects[index]->plane.position[1] = value[1]*(-1);
                objects[index]->plane.position[2] = value[2];
                //printf("got to position(plane)...");
            }
            else if(objects[index]->kind == 1){
                objects[index]->sphere.position[0] = value[0];
                //printf("Position (sphere) value 1: %f\n", value[0]);
                objects[index]->sphere.position[1] = value[1]*(-1);
                //printf("Position (sphere) value 2: %f\n", value[1]);
                objects[index]->sphere.position[2] = value[2];
                //printf("Position (sphere) value 3: %f\n", value[2]);
                //printf("got to position(sphere)...");
            }else{
                printf("THIS WENT WRONG");
            }

      }
         else if(strcmp(key, "normal") == 0){
            if(objects[index]->kind == 0){
                //printf("Starting normal: \n");
                objects[index]->plane.normal[0] = value[0];
                //printf("first value: %f\n", value[0]);
                objects[index]->plane.normal[1] = value[1];
                //printf("second value: %f\n", value[1]);
                //printf("third value: %f\n", value[2]);
                objects[index]->plane.normal[2] = value[2];
                //printf("got to normal...");

            }else{
                printf("This went TERRIBLY wrong...");
            }
          }
	  }else {
	    fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
		    key, line);
	    //char* value = next_string(json);
	  }
	  skip_ws(json);
	} else {
	  fprintf(stderr, "Error: Unexpected value on line %d\n", line);
	  exit(1);
	}
      }
      //printf("C is: %c \n", c);
      skip_ws(json);
      //printf("C is: %c \n", c);
      c = next_c(json);
      //printf("C is: %c \n", c);
      if (c == ',') {
        // noop
        skip_ws(json);
      } else if (c == ']') {
        //printf("Closing File....");
        fclose(json);
        objects[index+1] = NULL;
        //printf("Kind: %d \n", objects[index-2]->kind);
        return objects;
      } else {
	fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
	exit(1);
      }
    }
    index++;
  }

}
/////////////////////////////////////////////////////////////////////////////////////


static inline double sqr(double v) {
  return v*v;
}

static inline void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

double sphere_intersection(double* Ro, double* Rd,
			     double* C, double r) {

    double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
    double b = (2*(Rd[0]*(Ro[0] - C[0]) + Rd[1]*(Ro[1] - C[1]) + Rd[2]*(Ro[2] - C[2])));
    double c = sqr(Ro[0] - C[0]) + sqr(Ro[1] - C[1]) + sqr(Ro[2] - C[2]) - sqr(r);

    double det = sqr(b) - 4 *a*c;

    //printf("%f ", det);

  if (det < 0){return -1;}

    //printf("test");
    det = sqrt(det);

  double t0 = (-b - det / (2*a));
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;

}

double plane_intersection(double* Ro, double* Rd, double* L, double* N){
    double D;
    double t;

    for(int i = 0; i <3; i++){
        D += -1*L[i]*N[i];
    }

    t = -(N[0]*Ro[0] + N[1]*Ro[1] + N[2]*Ro[2] + D)/(N[0]*Rd[0] + N[1]*Rd[1] + N[2]*Rd[2]);

    return t;

}

Pixel* raycast(Object** scene){
    Pixel* buffer;


    double cx = 0;
    double cy = 0;
    double h = cam_height;
    double w = cam_width;

    int M = width; //width
    int N = height; //height

    width = M;
    height = N;

    buffer = malloc(sizeof(Pixel) * M * N);

    double pixheight = h / M;
    double pixwidth = w / N;

    int pixcount = 0;
    for (int y = 0; y < M; y += 1) {
    for (int x = 0; x < N; x += 1) {
      double Ro[3] = {0, 0, 0};
      double Rd[3] = {
        (cx - (w/2) + pixwidth * (x + 0.5)),
        (cy - (h/2) + pixheight * (y + 0.5)),
        1
      };
      normalize(Rd);

      double best_t = INFINITY;
      double* color;
      double* best_color;
    for (int i=0; scene[i] != 0; i += 1) {
    double t = 0;


    color = scene[i]->color;
	switch(scene[i]->kind) {
	case 1:
	  t = sphere_intersection(Ro, Rd,
				    scene[i]->sphere.position,
				    scene[i]->sphere.radius);
				    //printf("%f", t);
	  break;
    case 0:
        t = plane_intersection(Ro, Rd,
                     scene[i]->plane.position,
                     scene[i]->plane.normal);
        break;
	default:
	  // Horrible error
	  printf("horrible error");
	  exit(1);
	}


	if (t > 0 && t < best_t){
        best_t = t;
        best_color = color;
	}
      }
      if (best_t > 0 && best_t != INFINITY) {
            //printf("%c", best_color); //hit
            buffer[pixcount].r = best_color[0]*255;
            buffer[pixcount].g = best_color[1]*255;
            buffer[pixcount].b = best_color[2]*255;
      } else {
            buffer[pixcount].r = 0;
            buffer[pixcount].g = 0;
            buffer[pixcount].b = 0;
      }
        pixcount++;
        //printf("%d", pixcount);
    }
    //printf("\n");
  }
  return buffer;
}

int writeP6(char* fname, Pixel* buffer){
    FILE* fh;
    fh = fopen(fname, "wb");

    fprintf(fh, "P6\n");
    fprintf(fh, "%d ", width);
    fprintf(fh, "%d\n", height);
    fprintf(fh, "%d\n", 255);

    int i;
    for(i = 0; i < width*height; i++){
        unsigned char* rgb;
        rgb = malloc(sizeof(unsigned char)*64);
        rgb[0] = buffer[i].r;
        rgb[1] = buffer[i].g;
        rgb[2] = buffer[i].b;
        fwrite(rgb, 1, 3, fh);

    };

    fclose(fh);


}

int main(int argc, char *argv[]){
    //printf("%d", argc);

    if(argc != 5){
        fprintf(stderr, "Incorrect usage of arguments: width height input.json output.ppm");
        return 1;
    }


    width = atoi(argv[1]);

    height = atoi(argv[2]);

    Object** objects;
    objects = malloc(sizeof(Object*)*144);

    Object** scene = read_scene(argv[3], objects);

    Pixel* buffer = raycast(scene);

    writeP6(argv[4], buffer);


  return 0;

}
