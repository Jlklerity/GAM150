#include <math.h>
#include <AEEngine.h>
#include "game.h"

AEGfxVertexList* CircleMesh(int slices, float radius, float angleStep, u32 color) {

	AEGfxMeshStart();
	for (int i = 0; i < slices; i++) {
		float angle1 = i * angleStep;
		float angle2 = (i + 1) * angleStep;

		//player's origin
		float x1 = 0.0f;
		float y1 = 0.0f;

		//one point on the rim
		float x2 = cosf(angle1) * radius;
		float y2 = sinf(angle1) * radius;

		//another point on the rim
		float x3 = cosf(angle2) * radius;
		float y3 = sinf(angle2) * radius;

		AEGfxTriAdd(x1, y1, color, 0.5f, 0.5f,
			x2, y2, color, 0.0f, 0.0f,
			x3, y3, color, 0.0f, 0.0f);
	}

	// Saving the mesh (list of triangles) in pMesh
	AEGfxVertexList* pMesh = AEGfxMeshEnd();
	if (!pMesh) {
		printf("Error creating circle mesh!\n");
		return 0;
	}
	return pMesh;
}

AEGfxVertexList* RectMesh(float width, float height, u32 color) {
	AEGfxMeshStart();

	float halfWidth = width / 2.0f;
	float halfHeight = height / 2.0f;

	AEGfxTriAdd(
		-halfWidth, -halfHeight, color, 0.0f, 1.0f,
		halfWidth, -halfHeight, color, 1.0f, 1.0f,
		-halfWidth, halfHeight, color, 0.0f, 0.0f);

	AEGfxTriAdd(
		halfWidth, -halfHeight, color, 1.0f, 1.0f,
		halfWidth, halfHeight, color, 1.0f, 0.0f,
		-halfWidth, halfHeight, color, 0.0f, 0.0f);
	AEGfxVertexList* pMesh = AEGfxMeshEnd();

	if (pMesh == NULL) {
		printf("Error creating rectangle mesh!\n");
		return 0;
	}
	return pMesh;
}

 
 