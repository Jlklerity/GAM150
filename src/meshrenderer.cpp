#include "meshrenderer.h"

void MeshRenderer(float objectX, float objectY, AEMtx33 objectScale, 
				  AEMtx33 objectRot, AEMtx33 objectTrans, AEGfxVertexList objectMesh) {
	
	AEMtx33 scale, rot, trans;
	AEMtx33Scale(&objectScale, 1.0f, 1.0f); //scaling
	AEMtx33Rot(&objectRot, 0.0f);  //rotation
	AEMtx33Trans(&objectTrans, objectX, objectY); //translation
	AEMtx33Concat(&objectTrans, &objectRot, &objectScale); //player = rot * scale
	AEMtx33Concat(&objectTrans, &trans, &plusHP); //player = (rot * scale) * translate

	AEGfxSetRenderMode(AE_GFX_RM_COLOR);
	AEGfxSetTransform(objectTrans.m);
	AEGfxMeshDraw(&objectMesh, AE_GFX_MDM_TRIANGLES);
}
 