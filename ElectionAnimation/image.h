png_byte black[3] = { 0, 0, 0 };
png_byte white[3] = { 255, 255, 255 };

void hsv2rgb(float h, float s, float v, png_byte *out)
{
    double      hh, p, q, t, ff;
    long        i;

    if(s <= 0.0) {       // < is bogus, just shuts up warnings
        out[0] = v * 255;
        out[1] = v * 255;
        out[2] = v * 255;
        return;
    }
    hh = h;
    if(hh >= 360.0) hh = 0.0;
    hh /= 60.0;
    i = (long)hh;
    ff = hh - i;
    p = v * (1.0 - s);
    q = v * (1.0 - (s * ff));
    t = v * (1.0 - (s * (1.0 - ff)));

    switch(i) {
    case 0:
        out[0] = v * 255;
        out[1] = t * 255;
        out[2] = p * 255;
        break;
    case 1:
        out[0] = q * 255;
        out[1] = v * 255;
        out[2] = p * 255;
        break;
    case 2:
        out[0] = p * 255;
        out[1] = v * 255;
        out[2] = t * 255;
        break;

    case 3:
        out[0] = p * 255;
        out[1] = q * 255;
        out[2] = v * 255;
        break;
    case 4:
        out[0] = t * 255;
        out[1] = p * 255;
        out[2] = v * 255;
        break;
    case 5:
    default:
        out[0] = v * 255;
        out[1] = p * 255;
        out[2] = q * 255;
        break;
    }
}


template<int width, int height> struct image
{
	png_byte data[height][width][3];

	void clear(png_byte *color)
	{
		for(int y = 0; y < height; y++)
		{
			for(int x = 0; x < width; x++)
			{
				data[y][x][0] = color[0];
				data[y][x][1] = color[1];
				data[y][x][2] = color[2];
			}
		}
	}

	inline void pixel(int x,  int y, png_byte *color)
	{
		if(x < 0 || y < 0 || x >= width || y >= height)
			return;
		data[y][x][0] = color[0];
		data[y][x][1] = color[1];
		data[y][x][2] = color[2];
	}

	void draw_box(int x, int y, int w, int h, png_byte *c)
	{
		draw_line(x, y, x + w, y, c);
		draw_line(x, y, x, y + h, c);
		draw_line(x + w, y, x + w, y + h, c);
		draw_line(x, y + w, x + w, y + h, c);
	}
	
	void draw_line(int x1,int y1,int x2,int y2,png_byte *c)
	{
		int x,y,dx,dy,dx1,dy1,px,py,xe,ye,i;
		dx=x2-x1;
		dy=y2-y1;
		dx1=fabs(dx);
		dy1=fabs(dy);
		px=2*dy1-dx1;
		py=2*dx1-dy1;
		if(dy1<=dx1)
		{
			if(dx>=0)
			{
				x=x1;
				y=y1;
				xe=x2;
			}
			else
			{
				x=x2;
				y=y2;
				xe=x1;
			}
			pixel(x,y,c);
			for(i=0;x<xe;i++)
			{
				x=x+1;
				if(px<0)
				{
					px=px+2*dy1;
				}
				else
				{
					if((dx<0 && dy<0) || (dx>0 && dy>0))
					{
						y=y+1;
					}
					else
					{
						y=y-1;
					}
					px=px+2*(dy1-dx1);
				}
				pixel(x,y,c);
			}
		}
		else
		{
			if(dy>=0)
			{
				x=x1;
				y=y1;
				ye=y2;
			}
			else
			{
				x=x2;
				y=y2;
				ye=y1;
			}
			pixel(x,y,c);
			for(i=0;y<ye;i++)
			{
				y=y+1;
				if(py<=0)
				{
					py=py+2*dx1;
				}
				else
				{
					if((dx<0 && dy<0) || (dx>0 && dy>0))
					{
						x=x+1;
					}
					else
					{
						x=x-1;
					}
					py=py+2*(dx1-dy1);
				}
				pixel(x,y,c);
			}
		}
	}

	void draw_circle(int x, int y, unsigned int radius, png_byte *color)
	{
		/*Bresenham alg.*/
		int xd, yd, i, d, mode;
		for(mode=0; mode<=1; mode++)
		{
			xd = 0;
			yd = radius;
			d = 3 - (2 * yd);
			while(yd >= xd)
			{
				if(mode==0)
				{ /*fill:*/
					for(i=x-xd+1; i<x+xd; i++)
						pixel(i, y + yd, color);
					for(i=x-xd+1; i<x+xd; i++)
						pixel(i, y - yd, color);
					for(i=x-yd+1; i<x+yd; i++)
						pixel(i, y + xd, color);
					for(i=x-yd+1; i<x+yd; i++)
						pixel(i, y - xd, color);
				}
				else
				{	/*border points in 8 octants:*/
					pixel(x - xd, y + yd, black);
					pixel(x + xd, y + yd, black);
					pixel(x - xd, y - yd, black);
					pixel(x + xd, y - yd, black);
					pixel(x - yd, y + xd, black);
					pixel(x + yd, y + xd, black);
					pixel(x - yd, y - xd, black);
					pixel(x + yd, y - xd, black);
				}
				/*update coords:*/
				if (d < 0)
				{
					d += 4*xd + 6;
				}
				else
				{
					d += 10 + 4*(xd-yd);
					yd--;
				}
				xd++;
			} /*end while*/
		}
	}

	int writeImage(const char* filename, char* title)
	{
		int code = 0;
		FILE *fp = NULL;
		png_structp png_ptr = NULL;
		png_infop info_ptr = NULL;
		png_bytep row = NULL;
		printf("Saving PNG %s\n", filename);
		
		// Open file for writing (binary mode)
		fp = fopen(filename, "wb");
		if (fp == NULL) {
			fprintf(stderr, "Could not open file %s for writing\n", filename);
			code = 1;
			goto finalise;
		}

		// Initialize write structure
		png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (png_ptr == NULL) {
			fprintf(stderr, "Could not allocate write struct\n");
			code = 1;
			goto finalise;
		}

		// Initialize info structure
		info_ptr = png_create_info_struct(png_ptr);
		if (info_ptr == NULL) {
			fprintf(stderr, "Could not allocate info struct\n");
			code = 1;
			goto finalise;
		}

		// Setup Exception handling
		if (setjmp(png_jmpbuf(png_ptr))) {
			fprintf(stderr, "Error during png creation\n");
			code = 1;
			goto finalise;
		}

		png_init_io(png_ptr, fp);

		// Write header (8 bit colour depth)
		png_set_IHDR(png_ptr, info_ptr, width, height,
				8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

		// Set title
		if (title != NULL) {
			png_text title_text;
			title_text.compression = PNG_TEXT_COMPRESSION_NONE;
			title_text.key = "Title";
			title_text.text = title;
			png_set_text(png_ptr, info_ptr, &title_text, 1);
		}

		png_write_info(png_ptr, info_ptr);

		// Write image data
		for (int y = 0 ; y<height ; y++)
			png_write_row(png_ptr, data[y][0]);

		// End write
		png_write_end(png_ptr, NULL);

		finalise:
		if (fp != NULL) fclose(fp);
		if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
		if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);

		return code;
	}
};

