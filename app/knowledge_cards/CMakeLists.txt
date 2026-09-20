# ##############################################################################
# packages/demos/knowledge_cards/CMakeLists.txt
# ##############################################################################

if(CONFIG_KNOWLEDGE_CARDS_DEMO)
  set(AI_AGENT_DIR ${NUTTX_APPS_DIR}/packages/ai_agent)
  set(KNOWLEDGE_CARDS_INCDIRS
    ${AI_AGENT_DIR}/include
    ${AI_AGENT_DIR}/src
    ${NUTTX_APPS_DIR}/netutils/cjson/cJSON
    ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/include
  )

  nuttx_add_application(
    NAME knowledge_cards
    PRIORITY ${CONFIG_KNOWLEDGE_CARDS_PRIORITY}
    STACKSIZE ${CONFIG_KNOWLEDGE_CARDS_STACKSIZE}
    MODULE ${CONFIG_KNOWLEDGE_CARDS_DEMO}
    SRCS
      src/knowledge_cards_main.c
      src/kc_proxy_stub.c
      src/kc_deck.c
      src/kc_voice.c
      src/kc_font_cjk_16.c
      src/kc_font_20.c
      src/kc_font_24.c
      ${AI_AGENT_DIR}/src/voice/audio_capture.c
      ${AI_AGENT_DIR}/src/voice/voice_asr.c
      ${AI_AGENT_DIR}/src/voice/volc_asr.c
      ${AI_AGENT_DIR}/src/infra/config_store.c
    INCLUDE_DIRECTORIES
      ${KNOWLEDGE_CARDS_INCDIRS}
    DEPENDS lvgl
  )
endif()
