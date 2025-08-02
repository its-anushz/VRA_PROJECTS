import * as THREE from 'three';
import { GLTFLoader } from 'three/examples/jsm/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { FlyControls } from 'three/examples/jsm/controls/FlyControls.js';
import { Sky } from 'three/examples/jsm/objects/Sky.js';
import { ConvexGeometry } from 'three/examples/jsm/geometries/ConvexGeometry.js';
import * as Ammo from "ammo.js";
import { Water } from 'three/examples/jsm/objects/Water.js';
import { DRACOLoader } from 'three/examples/jsm/loaders/DRACOLoader.js';

// initial set up of window
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.setClearColor(0x000000);
renderer.setPixelRatio(window.devicePixelRatio);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;

document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();

// Water setup with stormy effect
const waterGeometry = new THREE.PlaneGeometry(200, 200);
const water = new Water(waterGeometry, {
    textureWidth: 512,
    textureHeight: 512,
    waterNormals: new THREE.TextureLoader().load('https://threejs.org/examples/textures/waternormals.jpg', function (texture) {
        texture.wrapS = texture.wrapT = THREE.RepeatWrapping;
    }),
    sunDirection: new THREE.Vector3(),
    sunColor: 0xffffff,
    waterColor: 0x00dcd6,  // Darker water for stormy effect
    distortionScale: 10.0,  // Larger distortion for bigger waves
    fog: scene.fog !== undefined,
});
water.rotation.x = -Math.PI / 2;
water.position.set(-1, -1, -1);
scene.add(water);

// Sky setup (cloudy and stormy)
const sky = new Sky();
sky.scale.setScalar(10000);
scene.add(sky);

const skyUniforms = sky.material.uniforms;
skyUniforms['turbidity'].value = 50;  // Increase turbidity for more dense clouds
skyUniforms['rayleigh'].value = 0.5;  // Less scattering for a darker stormy sky
skyUniforms['mieCoefficient'].value = 0.1;  // Increase for a more diffused look
skyUniforms['mieDirectionalG'].value = 0.9;  // High value for more light scattering

const pmremGenerator = new THREE.PMREMGenerator(renderer);
const parameters = {
    elevation: 1,  // Low sun to enhance stormy effect
    azimuth: 200
};

// Update sun position based on sky parameters
function updateSun() {
    const phi = THREE.MathUtils.degToRad(90 - parameters.elevation);
    const theta = THREE.MathUtils.degToRad(parameters.azimuth);

    const sunPosition = new THREE.Vector3();
    sunPosition.setFromSphericalCoords(1, phi, theta);

    sky.material.uniforms['sunPosition'].value.copy(sunPosition);
    water.material.uniforms['sunDirection'].value.copy(sunPosition).normalize();

    scene.environment = pmremGenerator.fromScene(sky).texture;
}

updateSun();

// camera
const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);

// spotlight falling only on the island
const sunlight = new THREE.SpotLight(0xffffff, 50000, 0, 0.5, 1);

sunlight.position.set(50, 100, 50); // Positioning light as the sun at a high angle
sunlight.castShadow = true; // Enable shadow casting

sunlight.shadow.camera.near = 0.5; // Shadow camera settings
sunlight.shadow.camera.far = 500;
sunlight.shadow.camera.left = -50;
sunlight.shadow.camera.right = 50;
sunlight.shadow.camera.top = 50;
sunlight.shadow.camera.bottom = -50;

// Add sunlight to the scene
scene.add(sunlight);

// Ambient light for general illumination
const ambientLight = new THREE.AmbientLight(0xffffff, 0.5);
scene.add(ambientLight);


// Compressor
const boatloader = new GLTFLoader();
const loader = new GLTFLoader();

const dLoader=new DRACOLoader();
dLoader.setDecoderPath("https://www.gstatic.com/draco/versioned/decoders/1.5.7/");
dLoader.setDecoderConfig({type:"js"});
boatloader.setDRACOLoader(dLoader);
loader.setDRACOLoader(dLoader);


// Boat to escape in
let boatobj = new THREE.Box3();

boatloader.load('boat.glb', (gltf) => {
    const boatmesh = gltf.scene;

    boatobj = boatmesh;
    boatmesh.traverse((child) => {
        if (child.isMesh) {
            child.castShadow = true;
            child.receiveShadow = true;
            console.log(child);
        }
    });
    console.log(boatmesh);
    boatmesh.visible = false;	// will be made visible at the end of the game
    boatmesh.position.set(21, -0.3, -4);	// location at harbour
    boatmesh.rotation.y = Math.PI / 2;
    scene.add(boatmesh);
}, (xhr) => {
    console.log(`loading ${xhr.loaded / xhr.total * 100}%`);
}, (error) => {
    console.error(error);
});

const colliders = []; // Array to store colliders
let islandmodel = null;
let children = [];

// loading main island model made in blender
loader.load(
    'minimodel.glb',
    (gltf) => {
        console.log('loading model');
        const mesh = gltf.scene;
        islandmodel = gltf.scene;

        // Traverse the scene to process each object
        mesh.traverse((child) => {
            if (child.isMesh) {
                child.castShadow = true;
                child.receiveShadow = true;

                // Boundaries around the island are made invisible so that the player does not fall out of the island
                if (!child.name.startsWith("wall")) {
                    child.material.transparent = true;
                } else {
                    child.visible = false;
                }

                // No collider for a few objects
                if (child.name.startsWith("Object_7") || child.name.startsWith("Object_8")) {
                    console.log("");
                } else {
                    // For collider
                    child.geometry.computeBoundingBox(); // Ensure bounding box is calculated
                    const geometryCenter = new THREE.Vector3();
                    child.geometry.boundingBox.getCenter(geometryCenter); // Get the geometry center
                    child.geometry.center(); // Center the geometry around its origin

                    // Offset the object's position to maintain world-space alignment
                    child.position.add(geometryCenter);

                    // Compute the bounding box in world space
                    const boundingBox = new THREE.Box3().setFromObject(child);

                    // Add the bounding box to the colliders array
                    colliders.push({
                        obj: child,
                        box: boundingBox,
                        health: 5
                    });
                    children.push(child);
                }
            }
        });

        // Position and add the loaded model to the scene
        mesh.position.set(0, 0, 0);
        scene.add(mesh);

        // Hide progress container
        document.getElementById('progress-container').style.display = 'none';
    },
    (xhr) => {
        console.log(`loading ${xhr.loaded / xhr.total * 100}%`);
    },
    (error) => {
        console.error(error);
    }
);

console.log(children);



let isPointerLocked = false;
let rotationSpeed = 0.005; // Adjust rotation sensitivity
let characterRotation = 0; // Track current rotation

// Add event listener for pointer lock
document.addEventListener('click', () => {
    // Request pointer lock on canvas
    if (!isPointerLocked) {
        document.body.requestPointerLock();
    }
});

// Pointer lock change event
document.addEventListener('pointerlockchange', () => {
    isPointerLocked = document.pointerLockElement === document.body;
});

// Mouse movement handler
document.addEventListener('mousemove', (event) => {
    if (isPointerLocked) {
        const deltaX = event.movementX; // Movement along the X-axis
        characterRotation -= deltaX * rotationSpeed;
        character.rotation.y = characterRotation;
    }
});

// Movement speed
const speed = 0.1;
const moveSpeed = 0.1;
let armSwingDirection = 1; // 1 = swinging up, -1 = swinging down
let legAngle = 0;
let swingDirection = 1;
const jumpHeight = 0.1;
const gravity = -0.01;
let isJumping = false;
let jumpVelocity = 0;

// Add 'b' to keyState for boat start
const keyState = {
    character: { w: false, a: false, s: false, d: false, q: false, e: false, c: false, f: false, j: false, space: false, b: false },
};
let boatBuilt = false; // Track if boat is built
let characterInBoat = false; // Track if character is in the boat

window.addEventListener('keydown', (event) => {
    const key = event.key.toLowerCase(); // Normalize to lowercase
    if (key in keyState.character) keyState.character[key] = true;
    if (event.code === 'Space') {
        keyState.character.space = true;
        if (!isJumping) {
            isJumping = true;
            jumpVelocity = jumpHeight;
        }
    }
});

window.addEventListener('keyup', (event) => {
    const key = event.key.toLowerCase(); // Normalize to lowercase
    if (key in keyState.character) keyState.character[key] = false;
    if (event.code === 'Space') {
        keyState.character.space = false;
    }
});

let characterBox;

const gltfLoader = new GLTFLoader();
gltfLoader.load('character.glb', (gltf) => {
    character = gltf.scene;
    character.scale.set(0.2, 0.2, 0.2);
    character.position.set(12, 0, -3);

    // Find bones/meshes for head and arms
    character.traverse((child) => {
        if (child.name.toLowerCase().includes("head")) headBone = child;
        if (child.name.toLowerCase().includes("leftarm")) leftArm = child;
        if (child.name.toLowerCase().includes("rightarm")) rightArm = child;
        if (child.isMesh) {
            child.castShadow = true;
            child.receiveShadow = true;
        }
    });

    // Attach camera to head for FPV
    if (headBone) {
        headBone.add(camera);
        camera.position.set(0, 0.15, 0.15); // Adjust for your model
        camera.lookAt(new THREE.Vector3(0, 0.15, -1));
    } else {
        character.add(camera);
        camera.position.set(0, 1.5, 0.2);
    }

    scene.add(character);

    // Initialize characterBox now that character exists
    characterBox = new THREE.Box3().setFromObject(character);
}, undefined, (error) => {
    console.error('Error loading character.glb:', error);
});

// Animation loop
function animate() {
    requestAnimationFrame(animate);

    // Update water time if available
    if (water.material.uniforms['time']) {
        water.material.uniforms['time'].value += 1.0 / 60.0;
    }
    // Do NOT update sky.material.uniforms['time']

    // Character movement
    if (!character || !characterBox) return;

    const delta = 0.1; // Adjust this value for faster/slower movement

    // Front-back movement
    if (keyState.character.w) {
        character.position.z -= delta;
    }
    if (keyState.character.s) {
        character.position.z += delta;
    }

    // Left-right movement
    if (keyState.character.a) {
        character.position.x -= delta;
    }
    if (keyState.character.d) {
        character.position.x += delta;
    }

    // Boat movement
    if (keyState.character.b && !boatBuilt) {
        // Build and enter the boat
        boatBuilt = true;
        boatobj.visible = true;
        character.position.copy(boatobj.position);
        character.rotation.y = boatobj.rotation.y;
        characterInBoat = true;
    }

    if (characterInBoat) {
        // Move the boat
        if (keyState.character.w) {
            boatobj.position.z -= delta * 2; // Move boat forward
        }
        if (keyState.character.s) {
            boatobj.position.z += delta * 2; // Move boat backward
        }
        if (keyState.character.a) {
            boatobj.position.x -= delta * 2; // Move boat left
        }
        if (keyState.character.d) {
            boatobj.position.x += delta * 2; // Move boat right
        }
    }

    // Jumping
    if (isJumping) {
        character.position.y += jumpVelocity;
        jumpVelocity += gravity;

        // Land the character
        if (character.position.y <= 0) {
            character.position.y = 0;
            isJumping = false;
        }
    }

    // Update characterBox position
    characterBox.setFromObject(character);

    renderer.render(scene, camera);
}

animate();