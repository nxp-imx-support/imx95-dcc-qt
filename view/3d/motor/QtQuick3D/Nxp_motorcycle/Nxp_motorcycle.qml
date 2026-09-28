import QtQuick
import QtQuick3D

Node {
    id: node

    // Resources
    property url textureData: "maps/textureData.png"
    property url textureData24: "maps/textureData24.png"
    property url textureData22: "maps/textureData22.png"
    property url textureData12: "maps/textureData12.png"
    Texture {
        id: ____fbm_tietu02_01__png_texture
        objectName: "tietu0201.png"
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData12
    }
    Texture {
        id: ____fbm_tietu02_03__png_texture
        objectName: "tietu0202.png"
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData
    }
    Texture {
        id: ____fbm_tt_png_texture
        objectName: "tt.png"
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData22
    }
    Texture {
        id: ____fbm_tt3_png_texture
        objectName: "tt3.png"
        generateMipmaps: true
        mipFilter: Texture.Linear
        source: node.textureData24
    }
    PrincipledMaterial {
        id: ___004_material
        objectName: "material.004"
        baseColor: "#ffcccccc"
        baseColorMap: ____fbm_tietu02_01__png_texture
        roughness: 0.5
        normalMap: ____fbm_tietu02_03__png_texture
    }
    PrincipledMaterial {
        id: ___material
        objectName: "material"
        baseColor: "#fb0000"
        roughness: 0.4000000059604645
    }
    PrincipledMaterial {
        id: ___003_material
        objectName: "material.003"
        baseColor: "#83e4ff0a"
        roughness: 0.3104575276374817
    }
    PrincipledMaterial {
        id: ___002_material
        objectName: "material.002"
        baseColor: "#ffcccccc"
        baseColorMap: ____fbm_tt_png_texture
        roughness: 0.5
        normalMap: ____fbm_tt3_png_texture
    }
    PrincipledMaterial {
        id: faguang_material
        objectName: "faguang"
        baseColor: "#d7ffc90a"
        roughness: 0.4000000059604645
    }

    // Nodes:
    Node {
        id: rootNode
        objectName: "RootNode"
        Model {
            id: ___
            objectName: "front_ligth"
            x: 0.13506627082824707
            y: 54.11983871459961
            z: -61.719757080078125
            rotation: Qt.quaternion(0.707097, -0.707097, -0.00363883, -0.00363883)
            scale.x: 100
            scale.y: 100
            scale.z: 100
            source: "meshes/submesh_00_LOD_1_006_mesh.mesh"
            materials: [
                faguang_material
            ]
        }
        Model {
            id: ___5
            objectName: "rear_ligth"
            x: -1.4657728672027588
            y: 52.77141189575195
            z: 80.9127197265625
            rotation: Qt.quaternion(0.707097, -0.707097, -0.00363883, -0.00363883)
            scale.x: 100
            scale.y: 100
            scale.z: 100
            source: "meshes/submesh_00_LOD_1_004_mesh.mesh"
            materials: [
                ___material
            ]
        }
        Model {
            id: __
            objectName: "Baffle"
            x: -0.5192114114761353
            y: 35.36602020263672
            z: 1.8483977317810059
            rotation: Qt.quaternion(0.707097, -0.707097, -0.00363883, -0.00363883)
            scale.x: 100
            scale.y: 100
            scale.z: 100
            source: "meshes/submesh_00_LOD_1_001_mesh.mesh"
            materials: [
                ___004_material
            ]
        }
        Model {
            id: __15
            objectName: "main_light"
            x: 0.23133762180805206
            y: 53.59271240234375
            z: -70.9527816772461
            rotation: Qt.quaternion(0.707097, -0.707097, -0.00363883, -0.00363883)
            scale.x: 100
            scale.y: 100
            scale.z: 100
            source: "meshes/submesh_00_LOD_1_005_mesh.mesh"
            materials: [
                ___003_material
            ]
        }
        Model {
            id: __18
            objectName: "Body"
            x: -0.49820077419281006
            y: 26.164119720458984
            z: -0.24926285445690155
            rotation: Qt.quaternion(0.707097, -0.707097, -0.00363883, -0.00363883)
            scale.x: 100
            scale.y: 100
            scale.z: 100
            source: "meshes/submesh_00_LOD_1_002_mesh.mesh"
            materials: [
                ___002_material
            ]
        }
    }

    // Animations:
}
