ShadedMesh
==========
ShadedMesh is a lua object available that has "low-level" drawing capabilites that can be useful
for very custom rendering goals.

Example drawing a green rectangle in additive mode:

.. code-block:: lua

    local testMesh = gfx.CreateShadedMesh("guiColor")
    testMesh:SetParamVec4("color", 0.0, 0.7, 0.0, 1.0)
    testMesh:SetPrimitiveType(testMesh.PRIM_TRIFAN)
    testMesh:SetBlendMode(testMesh.BLEND_ADD)
    testMesh:SetData({
        {{0,0},{0,0}}, 
        {{50,0},{1,0}}, 
        {{50,50},{1,1}}, 
        {{0,50},{0,1}}, 
    })
    
    function render(deltaTime)
        testMesh:Draw()
    end
    
    
gfx.CreateShadedMesh(String material = "guiTex")
************************************************
Creates a new ShadedMesh object, the material is loaded from the skin shaders folder where
``material.fs`` and ``material.vs`` need to exist.

gfx.CreateShadedMesh(String material, String path)
************************************************
Creates a new ShadedMesh object, the material is loaded from the given path where
``material.fs`` and ``material.vs`` need to exist.

ShadedMesh:Draw()
*****************
Renders the ShadedMesh object.

ShadedMesh:AddTexture(String name, String path)
***********************************************
Adds a texture to the material that can be used in the shader code.

ShadedMesh:AddSkinTexture(String name, String path)
***************************************************
Same as AddTexture but it looks for the path using the skin texture folder as the root folder.

ShadedMesh:SetParam(String name, float value)
*********************************************
Sets the value of a named parameter in the material.

ShadedMesh:SetParamVec2(String name, float x, float y)
******************************************************
Sets the value of a named 2d vector parameter in the material.

ShadedMesh:SetParamVec3(String name, float x, float y, float z)
***************************************************************
Sets the value of a named 3d vector parameter in the material. Often used for setting RGB

ShadedMesh:SetParamVec4(String name, float x, float y, float z, float w)
************************************************************************
Sets the value of a named 4d vector parameter in the material. Often used for setting RGBA

ShadedMesh:SetData(Vertex[] data)
*************************************
Sets the geometry data for the ShadedMesh object. The format of a vertex is ``{{x,y}, {u,v}}``, see the example
at the beginning of this page for clarification.

ShadedMesh:SetBlendMode(int mode)
*********************************
Sets the blending mode for the ShadedMesh object. Available blending modes are::

    BLEND_NORM (default)
    BLEND_ADD
    BLEND_MULT
    
ShadedMesh:SetOpaque(bool opaque)
*********************************
Sets the material as opaque or non-opaque, non-opaque is the default.

ShadedMesh:SetPrimitiveType(int type)
*************************************
Sets the format of the geometry data supplied to the ShadedMesh. Available types are::

    PRIM_TRILIST (default)
    PRIM_TRIFAN
    PRIM_TRISTRIP
    PRIM_LINELIST
    PRIM_LINESTRIP
    PRIM_POINTLIST