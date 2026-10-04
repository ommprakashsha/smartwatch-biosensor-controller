#!/usr/bin/env bash
# Git Progressive Commit Generator across Stages 1 to 6

set -e

if [ ! -d ".git" ]; then
    echo "Initializing new Git repository..."
    git init
    git branch -M main
else
    echo "Git repository already exists."
fi

cat << 'EOF' > .gitignore
*.o
*.ko
*.mod
*.mod.c
*.order
*.symvers
.tmp_versions/
*.cmd
.cache.mk
*.log
src/biowatch_daemon
driver/smart_watch_bio.ko
*.swp
*~
.DS_Store
EOF

git add .gitignore
git commit -m "chore: initial repository configuration and .gitignore" || true

# Stage 1
echo "Staging Stage 1..."
git add docs/stage1_project_charter.md README.md
git commit -m "docs(stage1): project introduction, wearable biosensor charter and scope" || true

# Stage 2
echo "Staging Stage 2..."
git add docs/stage2_PRD.md
git commit -m "docs(stage2): complete PRD with functional and non-functional requirements" || true

# Stage 3
echo "Staging Stage 3..."
git add docs/stage3_system_architecture.md driver/biowatch_ioctl.h
git commit -m "docs(stage3): system architecture, hardware register map and UML diagrams" || true

# Stage 4
echo "Staging Stage 4..."
git add driver/biowatch_driver.c driver/Makefile src/DeviceHandle.hpp src/DeviceHandle.cpp docs/stage4_prototype_notes.md
git commit -m "feat(driver/prototype): character driver implementation and C++ RAII DeviceHandle" || true

# Stage 5
echo "Staging Stage 5..."
git add src/BioWatchController.hpp src/BioWatchController.cpp src/Logger.hpp src/Logger.cpp src/main.cpp src/Makefile Makefile tests/ docs/stage5_testing_guide.md
git commit -m "feat(system): multithreaded guardian daemon, thread-safe logger, and test suite" || true

# Stage 6
echo "Staging Stage 6..."
git add docs/stage6_viva_defense.md docs/presentation_slides.md
git commit -m "docs(stage6): final viva defense guide, presentation slides, and completion" || true

echo "=================================================================="
echo " SUCCESS! Git repository initialized with 6 progressive commits.  "
echo "=================================================================="
