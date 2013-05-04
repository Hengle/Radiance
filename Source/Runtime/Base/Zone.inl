// Zone.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

inline AddrSize zone_malloc_size(void *p) {
	return Zone::AllocSize(p);
}

inline void *zone_realloc(
	Zone &zone, 
	void *p,
	AddrSize size,
	AddrSize headerSize,
	AddrSize alignment
) {
	Zone *z = p ? Zone::FromPtr(p) : &zone;
	RAD_ASSERT(z==&zone);
	return z->Realloc(p, size, headerSize, alignment);
}

inline void *zone_malloc(
	Zone &zone,
	AddrSize size,
	AddrSize headerSize,
	AddrSize alignment
) {
	return zone.Realloc(0, size, headerSize, alignment);
}

inline void *zone_calloc(
	Zone &zone,
	AddrSize numElms,
	AddrSize elmSize,
	AddrSize headerSize,
	AddrSize alignment
) {
	void *p = zone_malloc(zone, numElms*elmSize, headerSize, alignment);
	if (p)
		memset(p, 0, numElms*elmSize);
	return p;
}

inline void *safe_zone_realloc(
	Zone &zone,
	void *p,
	AddrSize size,
	AddrSize headerSize,
	AddrSize alignment
) {
	p = zone_realloc(zone, p, size, headerSize, alignment);
	RAD_OUT_OF_MEM(p||!(size+headerSize));
	return p;
}

inline void *safe_zone_malloc(
	Zone &zone,
	AddrSize size,
	AddrSize headerSize,
	AddrSize alignment
) {
	void *p = zone_malloc(zone, size, headerSize, alignment);
	RAD_OUT_OF_MEM(p||!(size+headerSize));
	return p;
}

inline void *safe_zone_calloc(
	Zone &zone,
	AddrSize numElms,
	AddrSize elmSize,
	AddrSize headerSize,
	AddrSize alignment
) {
	void *p = zone_calloc(zone, numElms, elmSize, headerSize, alignment);
	RAD_OUT_OF_MEM(p||!(numElms*elmSize+headerSize));
	return p;
}

inline void zone_free(void *p) {
	Zone::Delete(p);
}

inline void zone_checkptr(void *p) {
#if defined (RAD_OPT_ZONE_MEMGUARD)
	if (p)
		Zone::CheckMemGuards(p);
#endif
}
