/* 
 * mprefixups.h: Handle left matra placement 
 *
 * Author: Sivaraj Doddannan
 * Ported from IBM's ICU engine.  Original copyright:
 * (C) Copyright IBM Corp. 1998-2003 - All Rights Reserved
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <pango/pango-types.h>
#include "mprefixups.h"
#include <stdio.h>

struct _FixupData
{
    glong fBaseIndex;
    glong fMPreIndex;
};

MPreFixups *indic_mprefixups_new(glong char_count)
{
    MPreFixups *mprefixups = g_new (MPreFixups, 1);
    mprefixups->fFixupCount = 0;
    mprefixups->fFixupData = g_new (FixupData, char_count);

    return mprefixups;
}

void indic_mprefixups_free (MPreFixups *mprefixups)
{
    g_free (mprefixups->fFixupData);
    g_free (mprefixups);
}

void indic_mprefixups_add (MPreFixups *mprefixups, glong baseIndex, glong mpreIndex)
{
    /* NOTE: don't add the fixup data if the mpre is right
     * before the base consonant glyph.
     */
    if (baseIndex - mpreIndex > 1) {
       mprefixups->fFixupData[mprefixups->fFixupCount].fBaseIndex = baseIndex;
       mprefixups->fFixupData[mprefixups->fFixupCount].fMPreIndex = mpreIndex;
    
       mprefixups->fFixupCount += 1;
    }
}

void indic_mprefixups_apply(MPreFixups *mprefixups, PangoGlyphString *glyphs)
{
    glong fixup;

    for (fixup = 0; fixup < mprefixups->fFixupCount; fixup += 1) {
	glong baseIndex = mprefixups->fFixupData[fixup].fBaseIndex;
	glong mpreIndex = mprefixups->fFixupData[fixup].fMPreIndex;
	glong mpreLimit, mpreCount, moveCount, mpreDest;
	glong i;
	PangoGlyph *mpreSave;
	int *clusterSave;

	/* determine post GSUB location of baseIndex and mpreIndex */
	gboolean no_base = TRUE;
	for (i = 0; i<glyphs->num_glyphs; i++) {
	    if (glyphs->log_clusters[i] == baseIndex) {
		baseIndex = i + 1;
		no_base = FALSE;
	    }
	    if (glyphs->log_clusters[i] == mpreIndex)
		mpreIndex = i;
	}
	if (no_base)
	    break;

	mpreLimit = mpreIndex + 1;

	while (glyphs->glyphs[baseIndex].glyph == 0xFFFF || glyphs->glyphs[baseIndex].glyph == 0xFFFE) {
	    baseIndex -= 1;
	}

	while (glyphs->glyphs[mpreIndex].glyph == 0xFFFF || glyphs->glyphs[mpreIndex].glyph == 0xFFFE) {
	    mpreLimit += 1;
	}
      
	if (mpreLimit == baseIndex) {
	    continue;
	}

	mpreCount  = mpreLimit - mpreIndex;
	moveCount  = baseIndex - mpreLimit;
	mpreDest   = baseIndex - mpreCount - 1;

	mpreSave    = g_new (PangoGlyph, mpreCount);
	clusterSave = g_new (int, mpreCount);

	for (i = 0; i < mpreCount; i += 1) {
	    mpreSave[i] = glyphs->glyphs[mpreIndex + i].glyph;
	    clusterSave[i] = glyphs->log_clusters[mpreIndex + i];
	}

	for (i = 0; i < moveCount; i += 1) {
	    glyphs->glyphs[mpreIndex + i].glyph = glyphs->glyphs[mpreLimit + i].glyph;
	    glyphs->log_clusters[mpreIndex + i] = glyphs->log_clusters[mpreLimit + i];
	}

	for (i = 0; i < mpreCount; i += 1) {
	    glyphs->glyphs[mpreDest + i].glyph = mpreSave[i];
	    glyphs->log_clusters[mpreDest + i] = clusterSave[i];
	}
 
	g_free(mpreSave);
    } 
}
