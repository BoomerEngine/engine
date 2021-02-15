shader TextPrinter
{
	const uint ASCII_MINUS = 45;
	const uint ASCII_DOT = 46;
	const uint ASCII_DIGIT_0 = 48;

	const uint SPACING_WIDTH = 1;
	const uint CHAR_WIDTH = 4 + SPACING_WIDTH;
	const uint DOT_WIDTH = 1 + SPACING_WIDTH;
	const uint CHAR_HEIGHT = 7;
	
	const uint[12] FontValues = array(
		261724575,
		71583076,
		252856470,
		126384263,
		143194521,
		126382367,
		261746975,
		35932303,
		110717334,
		260635039,
		61440,
		64
	);

	bool SampleFont(uvec2 localCoord, uint asciiCode)
	{
		if (localCoord.y >= CHAR_HEIGHT)
			return false;

		uint fontWidth = (ASCII_DOT == asciiCode) ? 1 : 4;
		uint valueIndex = (ASCII_DOT == asciiCode) ? 11 : (ASCII_MINUS == asciiCode ? 10 : asciiCode - ASCII_DIGIT_0);
		uint valueBits = FontValues[valueIndex];

		int localCoordIndex = fontWidth*localCoord.y + localCoord.x;
		int localCoordIndexBit = (1 << localCoordIndex);
		return localCoord.x < fontWidth && 0 != (localCoordIndexBit & valueBits);
	}

	int DisplayValueWidth(int value)
	{
		bool isNegative = value < 0;
		value = abs( value );
	
		int totalWidth = 0;
		{
			if (isNegative)
				totalWidth += CHAR_WIDTH;

			int v = value;
			for ( ;; )
			{				
				int currChar = ASCII_DIGIT_0 + v % 10;
				totalWidth += CHAR_WIDTH;
				v /= 10;
				if ( 0 == v )
					break;
			}
		}

		return totalWidth;
	}

	bool DisplayValueEx(ivec2 pixelCoord, ivec2 displayCoord, int value, int size, int alignHoriz, int alignVert, out ivec2 outCornerLT, out ivec2 outSize)
	{
		bool isNegative = value < 0;

		 int totalWidth = DisplayValueWidth( value );
		int totalHeight = CHAR_HEIGHT;
	
		value = abs( value );

		// Adjust display coord to alignment
		{
			if ( alignHoriz < 0 )
				displayCoord.x -= totalWidth * size;
			if ( alignHoriz == 0 )
				displayCoord.x -= ( totalWidth * size ) / 2;
			if ( alignVert < 0 )
				displayCoord.y -= totalHeight * size;
			if ( alignVert == 0 )
				displayCoord.y -= ( totalHeight * size ) / 2;
		}

		// Output size
		outCornerLT = displayCoord;
		outSize = size * ivec2(totalWidth, totalHeight);

		// Early exit
		if (any(pixelCoord < displayCoord) || any(pixelCoord > displayCoord + size*ivec2(totalWidth, totalHeight)))
			return false;

		// Display value
		{
			int charsCount = totalWidth / CHAR_WIDTH;
			ivec2 crd = ( pixelCoord - displayCoord ) / size;
			int v = value;
			int currChar = ASCII_DIGIT_0 + v % 10;
			int offset = -totalWidth + CHAR_WIDTH;
			
			if (isNegative)
			{
				--charsCount;
				
				if (SampleFont(crd, ASCII_MINUS))
					return true;
			}

			for (int i = 0; i < charsCount; ++i)
			{
				if (SampleFont( crd + ivec2(offset, 0), currChar))
					return true;
				
				v /= 10;
				currChar = ASCII_DIGIT_0 + v % 10;
				offset += CHAR_WIDTH;
			}
			
			return false;
		}

		return false;
	}
	
	bool DisplayValue(ivec2 pixelCoord, ivec2 displayCoord, int value, int size, int alignHoriz, int alignVert)
	{
		ivec2 tmp0, tmp1;
		return DisplayValueEx(pixelCoord, displayCoord, value, size, alignHoriz, alignVert, tmp0, tmp1);
	}

}