#include "meshrenderer.h"

void Mesh_Renderer(AEGfxVertexList* pMesh, float x, float y, float scaleX, 
					float scaleY, float rotation) {
	
    AEMtx33 scale, rot, trans, transform;

    AEMtx33Scale(&scale, scaleX, scaleY);
    AEMtx33Rot(&rot, rotation);
    AEMtx33Trans(&trans, x, y);
    AEMtx33Concat(&transform, &rot, &scale);     
    AEMtx33Concat(&transform, &trans, &transform); 

    AEGfxSetRenderMode(AE_GFX_RM_COLOR);
    AEGfxSetTransform(transform.m);
    AEGfxMeshDraw(pMesh, AE_GFX_MDM_TRIANGLES);
}

 
 