# 音频质量评估工具 Makefile

PYTHON = python3
TEST_DIR = test_audio
RESULTS_DIR = results

# 默认目标
.PHONY: help
help:
	@echo "音频质量评估工具"
	@echo "==================="
	@echo ""
	@echo "可用命令:"
	@echo "  make install      - 安装依赖"
	@echo "  make test         - 运行测试"
	@echo "  make clean        - 清理测试文件"
	@echo "  make demo         - 运行演示"
	@echo ""
	@echo "评估命令:"
	@echo "  make eval REF=ref.wav DEG=deg.ogg     - 单文件评估"
	@echo "  make batch REFDIR=refs DEGDIR=degs   - 批量评估"
	@echo ""
	@echo "示例:"
	@echo "  make eval REF=original.wav DEG=encoded.ogg"
	@echo "  make batch REFDIR=original_audio DEGDIR=encoded_audio"

# 安装依赖
.PHONY: install
install:
	@echo "安装Python依赖..."
	chmod +x install_dependencies.sh
	./install_dependencies.sh

# 运行测试
.PHONY: test
test:
	@echo "运行音频质量评估工具测试..."
	$(PYTHON) test_quality_tools.py

# 清理测试文件
.PHONY: clean
clean:
	@echo "清理测试文件..."
	rm -rf $(TEST_DIR)
	rm -rf $(RESULTS_DIR)
	rm -rf batch_test_results
	rm -f test_result_*.json
	rm -f advanced_test_result.json
	rm -f *.pyc
	rm -rf __pycache__

# 单文件评估（简化版）
.PHONY: eval
eval:
ifndef REF
	@echo "错误: 请指定参考文件 REF=filename"
	@echo "用法: make eval REF=reference.wav DEG=degraded.ogg"
	@exit 1
endif
ifndef DEG
	@echo "错误: 请指定待测文件 DEG=filename"
	@echo "用法: make eval REF=reference.wav DEG=degraded.ogg"
	@exit 1
endif
	@echo "评估音频质量: $(REF) vs $(DEG)"
	mkdir -p $(RESULTS_DIR)
	$(PYTHON) simple_quality_evaluator.py "$(REF)" "$(DEG)" -o "$(RESULTS_DIR)/evaluation_result.json"

# 单文件评估（完整版）
.PHONY: eval-advanced
eval-advanced:
ifndef REF
	@echo "错误: 请指定参考文件 REF=filename"
	@exit 1
endif
ifndef DEG
	@echo "错误: 请指定待测文件 DEG=filename"
	@exit 1
endif
	@echo "高级音频质量评估: $(REF) vs $(DEG)"
	mkdir -p $(RESULTS_DIR)
	$(PYTHON) audio_quality_evaluator.py "$(REF)" "$(DEG)" -o "$(RESULTS_DIR)/advanced_evaluation_result.json"

# 批量评估
.PHONY: batch
batch:
ifndef REFDIR
	@echo "错误: 请指定参考音频目录 REFDIR=dirname"
	@echo "用法: make batch REFDIR=references DEGDIR=degraded"
	@exit 1
endif
ifndef DEGDIR
	@echo "错误: 请指定待测音频目录 DEGDIR=dirname"
	@echo "用法: make batch REFDIR=references DEGDIR=degraded"
	@exit 1
endif
	@echo "批量评估: $(REFDIR) vs $(DEGDIR)"
	mkdir -p $(RESULTS_DIR)
	$(PYTHON) batch_quality_evaluator.py -r "$(REFDIR)" -d "$(DEGDIR)" -o "$(RESULTS_DIR)/batch_results"

# 演示
.PHONY: demo
demo:
	@echo "运行音频质量评估演示..."
	@echo "1. 生成测试音频文件..."
	$(PYTHON) test_quality_tools.py
	@echo ""
	@echo "2. 运行单文件评估演示..."
	@if [ -f "$(TEST_DIR)/original.wav" ] && [ -f "$(TEST_DIR)/noise_degraded.wav" ]; then \
		echo "评估原始文件 vs 噪声劣化文件:"; \
		$(PYTHON) simple_quality_evaluator.py "$(TEST_DIR)/original.wav" "$(TEST_DIR)/noise_degraded.wav"; \
	fi

# 检查工具状态
.PHONY: check
check:
	@echo "检查工具和依赖状态..."
	@echo ""
	@echo "Python版本:"
	@$(PYTHON) --version
	@echo ""
	@echo "Python库检查:"
	@$(PYTHON) -c "import sys; print('✓ Python可用')"
	@$(PYTHON) -c "import numpy; print('✓ NumPy已安装:', numpy.__version__)" 2>/dev/null || echo "✗ NumPy未安装"
	@$(PYTHON) -c "import scipy; print('✓ SciPy已安装:', scipy.__version__)" 2>/dev/null || echo "✗ SciPy未安装"
	@$(PYTHON) -c "import librosa; print('✓ Librosa已安装:', librosa.__version__)" 2>/dev/null || echo "✗ Librosa未安装"
	@$(PYTHON) -c "import soundfile; print('✓ SoundFile已安装:', soundfile.__version__)" 2>/dev/null || echo "✗ SoundFile未安装"
	@$(PYTHON) -c "import pesq; print('✓ PESQ已安装')" 2>/dev/null || echo "✗ PESQ未安装"
	@echo ""
	@echo "外部工具检查:"
	@which ffmpeg >/dev/null 2>&1 && echo "✓ FFmpeg已安装" || echo "✗ FFmpeg未安装"
	@which oggenc >/dev/null 2>&1 && echo "✓ oggenc已安装" || echo "✗ oggenc未安装 (可选)"
	@echo ""
	@echo "脚本文件检查:"
	@ls -la *.py | awk '{print "  " $$9 " (" $$5 " bytes)"}'

# 生成使用示例
.PHONY: examples
examples:
	@echo "音频质量评估工具使用示例"
	@echo "=========================="
	@echo ""
	@echo "1. 安装和设置:"
	@echo "   make install                    # 安装依赖"
	@echo "   make check                      # 检查状态"
	@echo "   make test                       # 运行测试"
	@echo ""
	@echo "2. 单文件评估:"
	@echo "   make eval REF=original.wav DEG=encoded.ogg"
	@echo "   make eval-advanced REF=test.wav DEG=compressed.ogg"
	@echo ""
	@echo "3. 批量评估:"
	@echo "   make batch REFDIR=original_audio DEGDIR=vorbis_encoded"
	@echo "   make batch REFDIR=refs DEGDIR=degs"
	@echo ""
	@echo "4. 直接使用Python脚本:"
	@echo "   python3 simple_quality_evaluator.py ref.wav deg.ogg"
	@echo "   python3 audio_quality_evaluator.py ref.wav deg.ogg -o results.json"
	@echo "   python3 batch_quality_evaluator.py -r refs/ -d degs/ -o results/"
	@echo ""
	@echo "5. 清理和维护:"
	@echo "   make clean                      # 清理测试文件"
	@echo "   make demo                       # 运行完整演示"

# 创建项目结构
.PHONY: init
init:
	@echo "初始化项目结构..."
	mkdir -p $(RESULTS_DIR)
	mkdir -p samples/original
	mkdir -p samples/encoded
	@echo "项目结构已创建:"
	@echo "  $(RESULTS_DIR)/          - 结果输出目录"
	@echo "  samples/original/   - 原始音频文件目录"
	@echo "  samples/encoded/    - 编码音频文件目录"
