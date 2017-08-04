xof 0302txt 0064
template Header {
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>
 WORD major;
 WORD minor;
 DWORD flags;
}

template Vector {
 <3D82AB5E-62DA-11cf-AB39-0020AF71E433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template Coords2d {
 <F6F23F44-7686-11cf-8F52-0040333594A3>
 FLOAT u;
 FLOAT v;
}

template Matrix4x4 {
 <F6F23F45-7686-11cf-8F52-0040333594A3>
 array FLOAT matrix[16];
}

template ColorRGBA {
 <35FF44E0-6C7C-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template ColorRGB {
 <D3E16E81-7835-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template IndexedColor {
 <1630B820-7842-11cf-8F52-0040333594A3>
 DWORD index;
 ColorRGBA indexColor;
}

template Boolean {
 <4885AE61-78E8-11cf-8F52-0040333594A3>
 WORD truefalse;
}

template Boolean2d {
 <4885AE63-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template MaterialWrap {
 <4885AE60-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template TextureFilename {
 <A42790E1-7810-11cf-8F52-0040333594A3>
 STRING filename;
}

template Material {
 <3D82AB4D-62DA-11cf-AB39-0020AF71E433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshFace {
 <3D82AB5F-62DA-11cf-AB39-0020AF71E433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template MeshFaceWraps {
 <4885AE62-78E8-11cf-8F52-0040333594A3>
 DWORD nFaceWrapValues;
 Boolean2d faceWrapValues;
}

template MeshTextureCoords {
 <F6F23F40-7686-11cf-8F52-0040333594A3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template MeshMaterialList {
 <F6F23F42-7686-11cf-8F52-0040333594A3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material]
}

template MeshNormals {
 <F6F23F43-7686-11cf-8F52-0040333594A3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template MeshVertexColors {
 <1630B821-7842-11cf-8F52-0040333594A3>
 DWORD nVertexColors;
 array IndexedColor vertexColors[nVertexColors];
}

template Mesh {
 <3D82AB44-62DA-11cf-AB39-0020AF71E433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

Header{
1;
0;
1;
}

Mesh {
 289;
 -5.00000;0.00000;5.00000;,
 -4.37500;0.00000;5.00000;,
 -4.37500;0.00000;4.37500;,
 -5.00000;0.00000;4.37500;,
 -3.75000;0.00000;5.00000;,
 -3.75000;0.00000;4.37500;,
 -3.12500;0.00000;5.00000;,
 -3.12500;0.00000;4.37500;,
 -2.50000;0.00000;5.00000;,
 -2.50000;0.00000;4.37500;,
 -1.87500;0.00000;5.00000;,
 -1.87500;0.00000;4.37500;,
 -1.25000;0.00000;5.00000;,
 -1.25000;0.00000;4.37500;,
 -0.62500;0.00000;5.00000;,
 -0.62500;0.00000;4.37500;,
 0.00000;0.00000;5.00000;,
 0.00000;0.00000;4.37500;,
 0.62500;0.00000;5.00000;,
 0.62500;0.00000;4.37500;,
 1.25000;0.00000;5.00000;,
 1.25000;0.00000;4.37500;,
 1.87500;0.00000;5.00000;,
 1.87500;0.00000;4.37500;,
 2.50000;0.00000;5.00000;,
 2.50000;0.00000;4.37500;,
 3.12500;0.00000;5.00000;,
 3.12500;0.00000;4.37500;,
 3.75000;0.00000;5.00000;,
 3.75000;0.00000;4.37500;,
 4.37500;0.00000;5.00000;,
 4.37500;0.00000;4.37500;,
 5.00000;0.00000;5.00000;,
 5.00000;0.00000;4.37500;,
 -4.37500;0.00000;3.75000;,
 -5.00000;0.00000;3.75000;,
 -3.75000;0.15000;3.75000;,
 -3.12500;0.15000;3.75000;,
 -2.50000;0.15000;3.75000;,
 -1.87500;0.15000;3.75000;,
 -1.25000;0.15000;3.75000;,
 -0.62500;0.15000;3.75000;,
 0.00000;0.15000;3.75000;,
 0.62500;0.15000;3.75000;,
 1.25000;0.15000;3.75000;,
 1.87500;0.15000;3.75000;,
 2.50000;0.15000;3.75000;,
 3.12500;0.15000;3.75000;,
 3.75000;0.15000;3.75000;,
 4.37500;0.00000;3.75000;,
 5.00000;0.00000;3.75000;,
 -4.37500;0.00000;3.12500;,
 -5.00000;0.00000;3.12500;,
 -3.75000;0.15000;3.12500;,
 -3.12500;0.27000;3.12500;,
 -2.50000;0.27000;3.12500;,
 -1.87500;0.27000;3.12500;,
 -1.25000;0.27000;3.12500;,
 -0.62500;0.27000;3.12500;,
 0.00000;0.27000;3.12500;,
 0.62500;0.27000;3.12500;,
 1.25000;0.27000;3.12500;,
 1.87500;0.27000;3.12500;,
 2.50000;0.27000;3.12500;,
 3.12500;0.27000;3.12500;,
 3.75000;0.15000;3.12500;,
 4.37500;0.00000;3.12500;,
 5.00000;0.00000;3.12500;,
 -4.37500;0.00000;2.50000;,
 -5.00000;0.00000;2.50000;,
 -3.75000;0.15000;2.50000;,
 -3.12500;0.27000;2.50000;,
 -2.50000;0.38000;2.50000;,
 -1.87500;0.38000;2.50000;,
 -1.25000;0.38000;2.50000;,
 -0.62500;0.38000;2.50000;,
 0.00000;0.38000;2.50000;,
 0.62500;0.38000;2.50000;,
 1.25000;0.38000;2.50000;,
 1.87500;0.38000;2.50000;,
 2.50000;0.38000;2.50000;,
 3.12500;0.27000;2.50000;,
 3.75000;0.15000;2.50000;,
 4.37500;0.00000;2.50000;,
 5.00000;0.00000;2.50000;,
 -4.37500;0.00000;1.87500;,
 -5.00000;0.00000;1.87500;,
 -3.75000;0.15000;1.87500;,
 -3.12500;0.27000;1.87500;,
 -2.50000;0.38000;1.87500;,
 -1.87500;0.48000;1.87500;,
 -1.25000;0.48000;1.87500;,
 -0.62500;0.48000;1.87500;,
 0.00000;0.48000;1.87500;,
 0.62500;0.48000;1.87500;,
 1.25000;0.48000;1.87500;,
 1.87500;0.48000;1.87500;,
 2.50000;0.38000;1.87500;,
 3.12500;0.27000;1.87500;,
 3.75000;0.15000;1.87500;,
 4.37500;0.00000;1.87500;,
 5.00000;0.00000;1.87500;,
 -4.37500;0.00000;1.25000;,
 -5.00000;0.00000;1.25000;,
 -3.75000;0.15000;1.25000;,
 -3.12500;0.27000;1.25000;,
 -2.50000;0.38000;1.25000;,
 -1.87500;0.48000;1.25000;,
 -1.25000;0.57000;1.25000;,
 -0.62500;0.57000;1.25000;,
 0.00000;0.57000;1.25000;,
 0.62500;0.57000;1.25000;,
 1.25000;0.57000;1.25000;,
 1.87500;0.48000;1.25000;,
 2.50000;0.38000;1.25000;,
 3.12500;0.27000;1.25000;,
 3.75000;0.15000;1.25000;,
 4.37500;0.00000;1.25000;,
 5.00000;0.00000;1.25000;,
 -4.37500;0.00000;0.62500;,
 -5.00000;0.00000;0.62500;,
 -3.75000;0.15000;0.62500;,
 -3.12500;0.27000;0.62500;,
 -2.50000;0.38000;0.62500;,
 -1.87500;0.48000;0.62500;,
 -1.25000;0.57000;0.62500;,
 -0.62500;0.62000;0.62500;,
 0.00000;0.62000;0.62500;,
 0.62500;0.62000;0.62500;,
 1.25000;0.57000;0.62500;,
 1.87500;0.48000;0.62500;,
 2.50000;0.38000;0.62500;,
 3.12500;0.27000;0.62500;,
 3.75000;0.15000;0.62500;,
 4.37500;0.00000;0.62500;,
 5.00000;0.00000;0.62500;,
 -4.37500;0.00000;0.00000;,
 -5.00000;0.00000;0.00000;,
 -3.75000;0.15000;0.00000;,
 -3.12500;0.27000;0.00000;,
 -2.50000;0.38000;0.00000;,
 -1.87500;0.48000;0.00000;,
 -1.25000;0.57000;0.00000;,
 -0.62500;0.62000;0.00000;,
 0.00000;0.62000;0.00000;,
 0.62500;0.62000;0.00000;,
 1.25000;0.57000;0.00000;,
 1.87500;0.48000;0.00000;,
 2.50000;0.38000;0.00000;,
 3.12500;0.27000;0.00000;,
 3.75000;0.15000;0.00000;,
 4.37500;0.00000;0.00000;,
 5.00000;0.00000;0.00000;,
 -4.37500;0.00000;-0.62500;,
 -5.00000;0.00000;-0.62500;,
 -3.75000;0.15000;-0.62500;,
 -3.12500;0.27000;-0.62500;,
 -2.50000;0.38000;-0.62500;,
 -1.87500;0.48000;-0.62500;,
 -1.25000;0.57000;-0.62500;,
 -0.62500;0.62000;-0.62500;,
 0.00000;0.62000;-0.62500;,
 0.62500;0.62000;-0.62500;,
 1.25000;0.57000;-0.62500;,
 1.87500;0.48000;-0.62500;,
 2.50000;0.38000;-0.62500;,
 3.12500;0.27000;-0.62500;,
 3.75000;0.15000;-0.62500;,
 4.37500;0.00000;-0.62500;,
 5.00000;0.00000;-0.62500;,
 -4.37500;0.00000;-1.25000;,
 -5.00000;0.00000;-1.25000;,
 -3.75000;0.15000;-1.25000;,
 -3.12500;0.27000;-1.25000;,
 -2.50000;0.38000;-1.25000;,
 -1.87500;0.48000;-1.25000;,
 -1.25000;0.57000;-1.25000;,
 -0.62500;0.57000;-1.25000;,
 0.00000;0.57000;-1.25000;,
 0.62500;0.57000;-1.25000;,
 1.25000;0.57000;-1.25000;,
 1.87500;0.48000;-1.25000;,
 2.50000;0.38000;-1.25000;,
 3.12500;0.27000;-1.25000;,
 3.75000;0.15000;-1.25000;,
 4.37500;0.00000;-1.25000;,
 5.00000;0.00000;-1.25000;,
 -4.37500;0.00000;-1.87500;,
 -5.00000;0.00000;-1.87500;,
 -3.75000;0.15000;-1.87500;,
 -3.12500;0.27000;-1.87500;,
 -2.50000;0.38000;-1.87500;,
 -1.87500;0.48000;-1.87500;,
 -1.25000;0.48000;-1.87500;,
 -0.62500;0.48000;-1.87500;,
 0.00000;0.48000;-1.87500;,
 0.62500;0.48000;-1.87500;,
 1.25000;0.48000;-1.87500;,
 1.87500;0.48000;-1.87500;,
 2.50000;0.38000;-1.87500;,
 3.12500;0.27000;-1.87500;,
 3.75000;0.15000;-1.87500;,
 4.37500;0.00000;-1.87500;,
 5.00000;0.00000;-1.87500;,
 -4.37500;0.00000;-2.50000;,
 -5.00000;0.00000;-2.50000;,
 -3.75000;0.15000;-2.50000;,
 -3.12500;0.27000;-2.50000;,
 -2.50000;0.38000;-2.50000;,
 -1.87500;0.38000;-2.50000;,
 -1.25000;0.38000;-2.50000;,
 -0.62500;0.38000;-2.50000;,
 0.00000;0.38000;-2.50000;,
 0.62500;0.38000;-2.50000;,
 1.25000;0.38000;-2.50000;,
 1.87500;0.38000;-2.50000;,
 2.50000;0.38000;-2.50000;,
 3.12500;0.27000;-2.50000;,
 3.75000;0.15000;-2.50000;,
 4.37500;0.00000;-2.50000;,
 5.00000;0.00000;-2.50000;,
 -4.37500;0.00000;-3.12500;,
 -5.00000;0.00000;-3.12500;,
 -3.75000;0.15000;-3.12500;,
 -3.12500;0.27000;-3.12500;,
 -2.50000;0.27000;-3.12500;,
 -1.87500;0.27000;-3.12500;,
 -1.25000;0.27000;-3.12500;,
 -0.62500;0.27000;-3.12500;,
 0.00000;0.27000;-3.12500;,
 0.62500;0.27000;-3.12500;,
 1.25000;0.27000;-3.12500;,
 1.87500;0.27000;-3.12500;,
 2.50000;0.27000;-3.12500;,
 3.12500;0.27000;-3.12500;,
 3.75000;0.15000;-3.12500;,
 4.37500;0.00000;-3.12500;,
 5.00000;0.00000;-3.12500;,
 -4.37500;0.00000;-3.75000;,
 -5.00000;0.00000;-3.75000;,
 -3.75000;0.15000;-3.75000;,
 -3.12500;0.15000;-3.75000;,
 -2.50000;0.15000;-3.75000;,
 -1.87500;0.15000;-3.75000;,
 -1.25000;0.15000;-3.75000;,
 -0.62500;0.15000;-3.75000;,
 0.00000;0.15000;-3.75000;,
 0.62500;0.15000;-3.75000;,
 1.25000;0.15000;-3.75000;,
 1.87500;0.15000;-3.75000;,
 2.50000;0.15000;-3.75000;,
 3.12500;0.15000;-3.75000;,
 3.75000;0.15000;-3.75000;,
 4.37500;0.00000;-3.75000;,
 5.00000;0.00000;-3.75000;,
 -4.37500;0.00000;-4.37500;,
 -5.00000;0.00000;-4.37500;,
 -3.75000;0.00000;-4.37500;,
 -3.12500;0.00000;-4.37500;,
 -2.50000;0.00000;-4.37500;,
 -1.87500;0.00000;-4.37500;,
 -1.25000;0.00000;-4.37500;,
 -0.62500;0.00000;-4.37500;,
 0.00000;0.00000;-4.37500;,
 0.62500;0.00000;-4.37500;,
 1.25000;0.00000;-4.37500;,
 1.87500;0.00000;-4.37500;,
 2.50000;0.00000;-4.37500;,
 3.12500;0.00000;-4.37500;,
 3.75000;0.00000;-4.37500;,
 4.37500;0.00000;-4.37500;,
 5.00000;0.00000;-4.37500;,
 -4.37500;0.00000;-5.00000;,
 -5.00000;0.00000;-5.00000;,
 -3.75000;0.00000;-5.00000;,
 -3.12500;0.00000;-5.00000;,
 -2.50000;0.00000;-5.00000;,
 -1.87500;0.00000;-5.00000;,
 -1.25000;0.00000;-5.00000;,
 -0.62500;0.00000;-5.00000;,
 0.00000;0.00000;-5.00000;,
 0.62500;0.00000;-5.00000;,
 1.25000;0.00000;-5.00000;,
 1.87500;0.00000;-5.00000;,
 2.50000;0.00000;-5.00000;,
 3.12500;0.00000;-5.00000;,
 3.75000;0.00000;-5.00000;,
 4.37500;0.00000;-5.00000;,
 5.00000;0.00000;-5.00000;;
 
 256;
 4;0,1,2,3;,
 4;1,4,5,2;,
 4;4,6,7,5;,
 4;6,8,9,7;,
 4;8,10,11,9;,
 4;10,12,13,11;,
 4;12,14,15,13;,
 4;14,16,17,15;,
 4;16,18,19,17;,
 4;18,20,21,19;,
 4;20,22,23,21;,
 4;22,24,25,23;,
 4;24,26,27,25;,
 4;26,28,29,27;,
 4;28,30,31,29;,
 4;30,32,33,31;,
 4;3,2,34,35;,
 4;2,5,36,34;,
 4;5,7,37,36;,
 4;7,9,38,37;,
 4;9,11,39,38;,
 4;11,13,40,39;,
 4;13,15,41,40;,
 4;15,17,42,41;,
 4;17,19,43,42;,
 4;19,21,44,43;,
 4;21,23,45,44;,
 4;23,25,46,45;,
 4;25,27,47,46;,
 4;27,29,48,47;,
 4;29,31,49,48;,
 4;31,33,50,49;,
 4;35,34,51,52;,
 4;34,36,53,51;,
 4;36,37,54,53;,
 4;37,38,55,54;,
 4;38,39,56,55;,
 4;39,40,57,56;,
 4;40,41,58,57;,
 4;41,42,59,58;,
 4;42,43,60,59;,
 4;43,44,61,60;,
 4;44,45,62,61;,
 4;45,46,63,62;,
 4;46,47,64,63;,
 4;47,48,65,64;,
 4;48,49,66,65;,
 4;49,50,67,66;,
 4;52,51,68,69;,
 4;51,53,70,68;,
 4;53,54,71,70;,
 4;54,55,72,71;,
 4;55,56,73,72;,
 4;56,57,74,73;,
 4;57,58,75,74;,
 4;58,59,76,75;,
 4;59,60,77,76;,
 4;60,61,78,77;,
 4;61,62,79,78;,
 4;62,63,80,79;,
 4;63,64,81,80;,
 4;64,65,82,81;,
 4;65,66,83,82;,
 4;66,67,84,83;,
 4;69,68,85,86;,
 4;68,70,87,85;,
 4;70,71,88,87;,
 4;71,72,89,88;,
 4;72,73,90,89;,
 4;73,74,91,90;,
 4;74,75,92,91;,
 4;75,76,93,92;,
 4;76,77,94,93;,
 4;77,78,95,94;,
 4;78,79,96,95;,
 4;79,80,97,96;,
 4;80,81,98,97;,
 4;81,82,99,98;,
 4;82,83,100,99;,
 4;83,84,101,100;,
 4;86,85,102,103;,
 4;85,87,104,102;,
 4;87,88,105,104;,
 4;88,89,106,105;,
 4;89,90,107,106;,
 4;90,91,108,107;,
 4;91,92,109,108;,
 4;92,93,110,109;,
 4;93,94,111,110;,
 4;94,95,112,111;,
 4;95,96,113,112;,
 4;96,97,114,113;,
 4;97,98,115,114;,
 4;98,99,116,115;,
 4;99,100,117,116;,
 4;100,101,118,117;,
 4;103,102,119,120;,
 4;102,104,121,119;,
 4;104,105,122,121;,
 4;105,106,123,122;,
 4;106,107,124,123;,
 4;107,108,125,124;,
 4;108,109,126,125;,
 4;109,110,127,126;,
 4;110,111,128,127;,
 4;111,112,129,128;,
 4;112,113,130,129;,
 4;113,114,131,130;,
 4;114,115,132,131;,
 4;115,116,133,132;,
 4;116,117,134,133;,
 4;117,118,135,134;,
 4;120,119,136,137;,
 4;119,121,138,136;,
 4;121,122,139,138;,
 4;122,123,140,139;,
 4;123,124,141,140;,
 4;124,125,142,141;,
 4;125,126,143,142;,
 4;126,127,144,143;,
 4;127,128,145,144;,
 4;128,129,146,145;,
 4;129,130,147,146;,
 4;130,131,148,147;,
 4;131,132,149,148;,
 4;132,133,150,149;,
 4;133,134,151,150;,
 4;134,135,152,151;,
 4;137,136,153,154;,
 4;136,138,155,153;,
 4;138,139,156,155;,
 4;139,140,157,156;,
 4;140,141,158,157;,
 4;141,142,159,158;,
 4;142,143,160,159;,
 4;143,144,161,160;,
 4;144,145,162,161;,
 4;145,146,163,162;,
 4;146,147,164,163;,
 4;147,148,165,164;,
 4;148,149,166,165;,
 4;149,150,167,166;,
 4;150,151,168,167;,
 4;151,152,169,168;,
 4;154,153,170,171;,
 4;153,155,172,170;,
 4;155,156,173,172;,
 4;156,157,174,173;,
 4;157,158,175,174;,
 4;158,159,176,175;,
 4;159,160,177,176;,
 4;160,161,178,177;,
 4;161,162,179,178;,
 4;162,163,180,179;,
 4;163,164,181,180;,
 4;164,165,182,181;,
 4;165,166,183,182;,
 4;166,167,184,183;,
 4;167,168,185,184;,
 4;168,169,186,185;,
 4;171,170,187,188;,
 4;170,172,189,187;,
 4;172,173,190,189;,
 4;173,174,191,190;,
 4;174,175,192,191;,
 4;175,176,193,192;,
 4;176,177,194,193;,
 4;177,178,195,194;,
 4;178,179,196,195;,
 4;179,180,197,196;,
 4;180,181,198,197;,
 4;181,182,199,198;,
 4;182,183,200,199;,
 4;183,184,201,200;,
 4;184,185,202,201;,
 4;185,186,203,202;,
 4;188,187,204,205;,
 4;187,189,206,204;,
 4;189,190,207,206;,
 4;190,191,208,207;,
 4;191,192,209,208;,
 4;192,193,210,209;,
 4;193,194,211,210;,
 4;194,195,212,211;,
 4;195,196,213,212;,
 4;196,197,214,213;,
 4;197,198,215,214;,
 4;198,199,216,215;,
 4;199,200,217,216;,
 4;200,201,218,217;,
 4;201,202,219,218;,
 4;202,203,220,219;,
 4;205,204,221,222;,
 4;204,206,223,221;,
 4;206,207,224,223;,
 4;207,208,225,224;,
 4;208,209,226,225;,
 4;209,210,227,226;,
 4;210,211,228,227;,
 4;211,212,229,228;,
 4;212,213,230,229;,
 4;213,214,231,230;,
 4;214,215,232,231;,
 4;215,216,233,232;,
 4;216,217,234,233;,
 4;217,218,235,234;,
 4;218,219,236,235;,
 4;219,220,237,236;,
 4;222,221,238,239;,
 4;221,223,240,238;,
 4;223,224,241,240;,
 4;224,225,242,241;,
 4;225,226,243,242;,
 4;226,227,244,243;,
 4;227,228,245,244;,
 4;228,229,246,245;,
 4;229,230,247,246;,
 4;230,231,248,247;,
 4;231,232,249,248;,
 4;232,233,250,249;,
 4;233,234,251,250;,
 4;234,235,252,251;,
 4;235,236,253,252;,
 4;236,237,254,253;,
 4;239,238,255,256;,
 4;238,240,257,255;,
 4;240,241,258,257;,
 4;241,242,259,258;,
 4;242,243,260,259;,
 4;243,244,261,260;,
 4;244,245,262,261;,
 4;245,246,263,262;,
 4;246,247,264,263;,
 4;247,248,265,264;,
 4;248,249,266,265;,
 4;249,250,267,266;,
 4;250,251,268,267;,
 4;251,252,269,268;,
 4;252,253,270,269;,
 4;253,254,271,270;,
 4;256,255,272,273;,
 4;255,257,274,272;,
 4;257,258,275,274;,
 4;258,259,276,275;,
 4;259,260,277,276;,
 4;260,261,278,277;,
 4;261,262,279,278;,
 4;262,263,280,279;,
 4;263,264,281,280;,
 4;264,265,282,281;,
 4;265,266,283,282;,
 4;266,267,284,283;,
 4;267,268,285,284;,
 4;268,269,286,285;,
 4;269,270,287,286;,
 4;270,271,288,287;;
 
 MeshMaterialList {
  2;
  256;
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0,
  1,
  0;;
  Material {
   1.000000;0.839216;0.403922;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.250000;0.209804;0.100980;;
  }
  Material {
   0.325490;0.792157;0.392157;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.081373;0.198039;0.098039;;
  }
 }
 MeshNormals {
  105;
  0.000000;1.000000;0.000000;,
  -0.028865;0.999166;0.028865;,
  -0.028966;0.995729;0.087662;,
  0.000000;0.993073;0.117500;,
  0.028966;0.995729;0.087662;,
  0.028865;0.999166;0.028865;,
  -0.087662;0.995729;0.028966;,
  -0.111277;0.987540;0.111277;,
  -0.023425;0.981955;0.187658;,
  0.000000;0.977482;0.211020;,
  0.023425;0.981955;0.187658;,
  0.111277;0.987540;0.111277;,
  0.087662;0.995729;0.028966;,
  -0.117500;0.993073;0.000000;,
  -0.187658;0.981955;0.023425;,
  -0.092428;0.991420;0.092428;,
  -0.021543;0.986983;0.159378;,
  0.000000;0.983492;0.180951;,
  0.021543;0.986983;0.159378;,
  0.092428;0.991420;0.092428;,
  0.187658;0.981955;0.023425;,
  0.117500;0.993073;0.000000;,
  -0.211020;0.977482;0.000000;,
  -0.159378;0.986983;0.021543;,
  -0.084791;0.992784;0.084791;,
  -0.019655;0.989090;0.145992;,
  0.000000;0.986182;0.165668;,
  0.019655;0.989091;0.145992;,
  0.084791;0.992784;0.084791;,
  0.159378;0.986983;0.021543;,
  0.211020;0.977482;0.000000;,
  -0.180951;0.983492;0.000000;,
  -0.145992;0.989090;0.019655;,
  -0.077096;0.994039;0.077096;,
  -0.017747;0.991024;0.132504;,
  0.000000;0.988646;0.150265;,
  0.017747;0.991024;0.132504;,
  0.077096;0.994039;0.077096;,
  0.145992;0.989091;0.019655;,
  0.180951;0.983492;0.000000;,
  -0.165668;0.986182;0.000000;,
  -0.132504;0.991024;0.017747;,
  -0.063486;0.995961;0.063486;,
  -0.009963;0.994809;0.101266;,
  0.000000;0.993799;0.111193;,
  0.009963;0.994809;0.101266;,
  0.063486;0.995961;0.063486;,
  0.132504;0.991024;0.017747;,
  0.165668;0.986182;0.000000;,
  -0.150265;0.988646;0.000000;,
  -0.101266;0.994809;0.009963;,
  -0.029921;0.999104;0.029921;,
  0.000000;0.999204;0.039904;,
  0.029921;0.999104;0.029921;,
  0.101266;0.994809;0.009963;,
  0.150265;0.988646;0.000000;,
  -0.111193;0.993799;0.000000;,
  -0.039904;0.999204;0.000000;,
  0.039904;0.999204;0.000000;,
  0.111193;0.993799;0.000000;,
  -0.101266;0.994809;-0.009963;,
  -0.029921;0.999104;-0.029921;,
  0.000000;0.999204;-0.039904;,
  0.029921;0.999104;-0.029921;,
  0.101266;0.994809;-0.009963;,
  -0.132504;0.991024;-0.017747;,
  -0.063486;0.995961;-0.063486;,
  -0.009963;0.994809;-0.101266;,
  0.000000;0.993799;-0.111193;,
  0.009963;0.994809;-0.101266;,
  0.063486;0.995961;-0.063486;,
  0.132504;0.991024;-0.017747;,
  -0.145992;0.989091;-0.019655;,
  -0.077096;0.994039;-0.077096;,
  -0.017747;0.991024;-0.132504;,
  0.000000;0.988646;-0.150265;,
  0.017747;0.991024;-0.132504;,
  0.077095;0.994038;-0.077095;,
  0.145992;0.989091;-0.019655;,
  -0.159378;0.986983;-0.021543;,
  -0.084791;0.992784;-0.084791;,
  -0.019655;0.989091;-0.145992;,
  0.000000;0.986182;-0.165668;,
  0.019655;0.989091;-0.145992;,
  0.084791;0.992784;-0.084791;,
  0.159378;0.986983;-0.021543;,
  -0.187658;0.981955;-0.023425;,
  -0.092428;0.991420;-0.092428;,
  -0.021543;0.986983;-0.159378;,
  0.000000;0.983492;-0.180951;,
  0.021543;0.986983;-0.159378;,
  0.092428;0.991420;-0.092428;,
  0.187658;0.981955;-0.023425;,
  -0.087662;0.995729;-0.028966;,
  -0.111277;0.987540;-0.111277;,
  -0.023425;0.981955;-0.187658;,
  0.000000;0.977482;-0.211020;,
  0.023425;0.981955;-0.187658;,
  0.111277;0.987540;-0.111277;,
  0.087662;0.995729;-0.028966;,
  -0.028865;0.999166;-0.028865;,
  -0.028966;0.995729;-0.087662;,
  0.000000;0.993073;-0.117500;,
  0.028966;0.995729;-0.087662;,
  0.028865;0.999166;-0.028865;;
  256;
  4;0,0,1,0;,
  4;0,0,2,1;,
  4;0,0,3,2;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,3,3;,
  4;0,0,4,3;,
  4;0,0,5,4;,
  4;0,0,0,5;,
  4;0,1,6,0;,
  4;1,2,7,6;,
  4;2,3,8,7;,
  4;3,3,9,8;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,9,9;,
  4;3,3,10,9;,
  4;3,4,11,10;,
  4;4,5,12,11;,
  4;5,0,0,12;,
  4;0,6,13,0;,
  4;6,7,14,13;,
  4;7,8,15,14;,
  4;8,9,16,15;,
  4;9,9,17,16;,
  4;9,9,17,17;,
  4;9,9,17,17;,
  4;9,9,17,17;,
  4;9,9,17,17;,
  4;9,9,17,17;,
  4;9,9,17,17;,
  4;9,9,18,17;,
  4;9,10,19,18;,
  4;10,11,20,19;,
  4;11,12,21,20;,
  4;12,0,0,21;,
  4;0,13,13,0;,
  4;13,14,22,13;,
  4;14,15,23,22;,
  4;15,16,24,23;,
  4;16,17,25,24;,
  4;17,17,26,25;,
  4;17,17,26,26;,
  4;17,17,26,26;,
  4;17,17,26,26;,
  4;17,17,26,26;,
  4;17,17,27,26;,
  4;17,18,28,27;,
  4;18,19,29,28;,
  4;19,20,30,29;,
  4;20,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,23,31,22;,
  4;23,24,32,31;,
  4;24,25,33,32;,
  4;25,26,34,33;,
  4;26,26,35,34;,
  4;26,26,35,35;,
  4;26,26,35,35;,
  4;26,26,36,35;,
  4;26,27,37,36;,
  4;27,28,38,37;,
  4;28,29,39,38;,
  4;29,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,32,40,31;,
  4;32,33,41,40;,
  4;33,34,42,41;,
  4;34,35,43,42;,
  4;35,35,44,43;,
  4;35,35,45,44;,
  4;35,36,46,45;,
  4;36,37,47,46;,
  4;37,38,48,47;,
  4;38,39,39,48;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,40,40,31;,
  4;40,41,49,40;,
  4;41,42,50,49;,
  4;42,43,51,50;,
  4;43,44,52,51;,
  4;44,45,53,52;,
  4;45,46,54,53;,
  4;46,47,55,54;,
  4;47,48,48,55;,
  4;48,39,39,48;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,40,40,31;,
  4;40,49,49,40;,
  4;49,50,56,49;,
  4;50,51,57,56;,
  4;51,52,0,57;,
  4;52,53,58,0;,
  4;53,54,59,58;,
  4;54,55,55,59;,
  4;55,48,48,55;,
  4;48,39,39,48;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,40,40,31;,
  4;40,49,49,40;,
  4;49,56,60,49;,
  4;56,57,61,60;,
  4;57,0,62,61;,
  4;0,58,63,62;,
  4;58,59,64,63;,
  4;59,55,55,64;,
  4;55,48,48,55;,
  4;48,39,39,48;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,40,40,31;,
  4;40,49,65,40;,
  4;49,60,66,65;,
  4;60,61,67,66;,
  4;61,62,68,67;,
  4;62,63,69,68;,
  4;63,64,70,69;,
  4;64,55,71,70;,
  4;55,48,48,71;,
  4;48,39,39,48;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,31,22;,
  4;31,40,72,31;,
  4;40,65,73,72;,
  4;65,66,74,73;,
  4;66,67,75,74;,
  4;67,68,75,75;,
  4;68,69,75,75;,
  4;69,70,76,75;,
  4;70,71,77,76;,
  4;71,48,78,77;,
  4;48,39,39,78;,
  4;39,30,30,39;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,22,13;,
  4;22,31,79,22;,
  4;31,72,80,79;,
  4;72,73,81,80;,
  4;73,74,82,81;,
  4;74,75,82,82;,
  4;75,75,82,82;,
  4;75,75,82,82;,
  4;75,76,82,82;,
  4;76,77,83,82;,
  4;77,78,84,83;,
  4;78,39,85,84;,
  4;39,30,30,85;,
  4;30,21,21,30;,
  4;21,0,0,21;,
  4;0,13,13,0;,
  4;13,22,86,13;,
  4;22,79,87,86;,
  4;79,80,88,87;,
  4;80,81,89,88;,
  4;81,82,89,89;,
  4;82,82,89,89;,
  4;82,82,89,89;,
  4;82,82,89,89;,
  4;82,82,89,89;,
  4;82,83,89,89;,
  4;83,84,90,89;,
  4;84,85,91,90;,
  4;85,30,92,91;,
  4;30,21,21,92;,
  4;21,0,0,21;,
  4;0,13,93,0;,
  4;13,86,94,93;,
  4;86,87,95,94;,
  4;87,88,96,95;,
  4;88,89,96,96;,
  4;89,89,96,96;,
  4;89,89,96,96;,
  4;89,89,96,96;,
  4;89,89,96,96;,
  4;89,89,96,96;,
  4;89,89,96,96;,
  4;89,90,96,96;,
  4;90,91,97,96;,
  4;91,92,98,97;,
  4;92,21,99,98;,
  4;21,0,0,99;,
  4;0,93,100,0;,
  4;93,94,101,100;,
  4;94,95,102,101;,
  4;95,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,96,102,102;,
  4;96,97,102,102;,
  4;97,98,103,102;,
  4;98,99,104,103;,
  4;99,0,0,104;,
  4;0,100,0,0;,
  4;100,101,0,0;,
  4;101,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,102,0,0;,
  4;102,103,0,0;,
  4;103,104,0,0;,
  4;104,0,0,0;;
 }
}
