let buildingMode = false;
let currentBuildType = null;
const gridSize = 4; // Grid cell size in world units
const wallHeight = 3; // Standard wall height

// Store all placed foundations for snapping checks
const foundations = [];
const walls = [];

// Toggle building menu
function toggleBuilding() {
    const buildDiv = document.getElementById('building');
    buildingMode = !buildingMode;
    buildDiv.style.display = buildingMode ? 'block' : 'none';
    buildDiv.innerHTML = `
        <button onclick="setBuildType('foundation')">Foundation</button>
        <button onclick="setBuildType('wall')">Wall</button>
        <button onclick="setBuildType('window')">Window</button>
        <button onclick="setBuildType('door')">Door</button>
        <button onclick="setBuildType('ceiling')">Ceiling</button>
    `;
}

// Set the current building type
function setBuildType(type) {
    currentBuildType = type;
}

// Snap position to world grid
function snapToGrid(position) {
    return new THREE.Vector3(
        Math.round(position.x / gridSize) * gridSize,
        position.y,
        Math.round(position.z / gridSize) * gridSize
    );
}

// Find nearest foundation edge for wall snapping
function snapToFoundationEdge(position) {
    let closestEdge = null;
    let minDistance = Infinity;

    for (let foundation of foundations) {
        const box = new THREE.Box3().setFromObject(foundation);
        const edges = [
            new THREE.Vector3(box.min.x, box.max.y, box.min.z), // Bottom-left
            new THREE.Vector3(box.max.x, box.max.y, box.min.z), // Bottom-right
            new THREE.Vector3(box.min.x, box.max.y, box.max.z), // Top-left
            new THREE.Vector3(box.max.x, box.max.y, box.max.z)  // Top-right
        ];

        for (let edge of edges) {
            const distance = position.distanceTo(edge);
            if (distance < minDistance && distance < gridSize / 2) {
                minDistance = distance;
                closestEdge = edge;
            }
        }
    }

    return closestEdge ? new THREE.Vector3(closestEdge.x, position.y, closestEdge.z) : position;
}

// Place a building object
function placeBuilding(type, position) {
    let geometry, material, mesh;

    if (type === 'foundation') {
        geometry = new THREE.BoxGeometry(gridSize, 0.5, gridSize);
        material = new THREE.MeshStandardMaterial({ color: 0x808080 });
        position = snapToGrid(position);
        mesh = new THREE.Mesh(geometry, material);
        mesh.position.copy(position);
        foundations.push(mesh);
    } else if (type === 'wall' || type === 'window' || type === 'door') {
        geometry = new THREE.BoxGeometry(gridSize, wallHeight, 0.2);
        material = new THREE.MeshStandardMaterial({ color: type === 'wall' ? 0x606060 : type === 'window' ? 0x87CEEB : 0x8B4513 });
        position = snapToFoundationEdge(position);
        mesh = new THREE.Mesh(geometry, material);
        mesh.position.copy(position);
        walls.push(mesh);
    } else if (type === 'ceiling') {
        geometry = new THREE.BoxGeometry(gridSize, 0.5, gridSize);
        material = new THREE.MeshStandardMaterial({ color: 0x808080 });
        position = snapToGrid(position);
        position.y = wallHeight; // Snap to wall height
        mesh = new THREE.Mesh(geometry, material);
        mesh.position.copy(position);
    }

    if (mesh) {
        scene.add(mesh);
    }
}

// Handle mouse click to place objects
document.addEventListener('mousedown', (event) => {
    if (!buildingMode || !currentBuildType) return;

    const raycaster = new THREE.Raycaster();
    const mouse = new THREE.Vector2(
        (event.clientX / window.innerWidth) * 2 - 1,
        -(event.clientY / window.innerHeight) * 2 + 1
    );
    raycaster.setFromCamera(mouse, camera);
    const intersects = raycaster.intersectObjects(scene.children);

    if (intersects.length > 0) {
        placeBuilding(currentBuildType, intersects[0].point);
    }
});

// Handle mouse wheel for rotation
document.addEventListener('wheel', (event) => {
    if (buildingMode && currentBuildType) {
        // Rotate preview (placeholder)
        console.log('Rotate building piece:', event.deltaY);
    }
});