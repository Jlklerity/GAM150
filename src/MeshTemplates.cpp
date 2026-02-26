#include "Game.h"
#include "MeshTemplates.h"

//Makes mesh of a circle
AEGfxVertexList* Circle_Mesh(int slices, float radius, float angleStep, u32 color) {

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

//Makes mesh of a rectangle
AEGfxVertexList* Rect_Mesh(float width, float height, u32 color) {
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

	if (!pMesh) {
		printf("Error creating rectangle mesh!\n");
		return 0;
	}
	return pMesh;
}

//Makes mesh for sprites
AEGfxVertexList* Sprite_Mesh(const f32 sprite_uv_width, const f32 sprite_uv_height) {
	AEGfxMeshStart();

	// This shape has 2 triangles that makes up a square
	// Color parameters represent colours as ARGB
	// UV coordinates to read from loaded textures
	AEGfxTriAdd(
		-0.5f, -0.5f, 0xFFFFFFFF, 0.0f, sprite_uv_height, // bottom left
		0.5f, -0.5f, 0xFFFFFFFF, sprite_uv_width, sprite_uv_height, // bottom right 
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // top left

	AEGfxTriAdd(
		0.5f, -0.5f, 0xFFFFFFFF, sprite_uv_width, sprite_uv_height, // bottom right 
		0.5f, 0.5f, 0xFFFFFFFF, sprite_uv_width, 0.0f,   // top right
		-0.5f, 0.5f, 0xFFFFFFFF, 0.0f, 0.0f);  // bottom left

	AEGfxVertexList* pMesh = AEGfxMeshEnd();


	if (!pMesh) {
		printf("Error creating sprite mesh!\n");
		return 0;
	}
	return pMesh;
}

AEGfxVertexList* Spritesheet_Mesh(float width, float height, float uMin, float vMin, float uMax, float vMax)
{
	AEGfxMeshStart();
	float w = width * 0.5f;
	float h = height * 0.5f;

	// Triangle 1 (Bottom-Left, Bottom-Right, Top-Left)
	AEGfxTriAdd(
		-w, -h, 0xFFFFFFFF, uMin, vMax,
		w, -h, 0xFFFFFFFF, uMax, vMax,
		-w, h, 0xFFFFFFFF, uMin, vMin
	);

	// Triangle 2 (Bottom-Right, Top-Right, Top-Left)
	AEGfxTriAdd(
		w, -h, 0xFFFFFFFF, uMax, vMax,
		w, h, 0xFFFFFFFF, uMax, vMin,
		-w, h, 0xFFFFFFFF, uMin, vMin
	);

	AEGfxVertexList* pMesh = AEGfxMeshEnd();


	if (!pMesh) {
		printf("Error creating spritesheet mesh!\n");
		return 0;
	}
	return pMesh;
}
