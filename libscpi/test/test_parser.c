/*
 * File:   test_parser.c
 * Author: Jan Breuer
 */

#include <stdio.h>
#include <stdlib.h>
#include "CUnit/Basic.h"

#include "scpi/scpi.h"

/*
 * CUnit Test Suite
 */

static scpi_result_t text_function(scpi_t* context) {
    char param[100];
    size_t param_len;

    if (!SCPI_ParamCopyText(context, param, 100, &param_len, TRUE)) {
        return SCPI_RES_ERR;
    }

    if (!SCPI_ParamCopyText(context, param, 100, &param_len, TRUE)) {
        return SCPI_RES_ERR;
    }

    SCPI_ResultText(context, param);

    return SCPI_RES_OK;
}

static scpi_result_t test_treeA(scpi_t* context) {

    SCPI_ResultInt(context, 10);

    return SCPI_RES_OK;
}

static scpi_result_t test_treeB(scpi_t* context) {

    SCPI_ResultInt(context, 20);

    return SCPI_RES_OK;
}

static const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    { .pattern = "*CLS", .callback = SCPI_CoreCls,},
    { .pattern = "*ESE", .callback = SCPI_CoreEse,},
    { .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
    { .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
    { .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
    { .pattern = "*OPC", .callback = SCPI_CoreOpc,},
    { .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
    { .pattern = "*RST", .callback = SCPI_CoreRst,},
    { .pattern = "*SRE", .callback = SCPI_CoreSre,},
    { .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
    { .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
    { .pattern = "*TST?", .callback = SCPI_CoreTstQ,},
    { .pattern = "*WAI", .callback = SCPI_CoreWai,},

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    { .pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
    { .pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
    { .pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

    { .pattern = "STATus:QUEStionable[:EVENt]?", .callback = SCPI_StatusQuestionableEventQ,},
    { .pattern = "STATus:QUEStionable:ENABle", .callback = SCPI_StatusQuestionableEnable,},
    { .pattern = "STATus:QUEStionable:ENABle?", .callback = SCPI_StatusQuestionableEnableQ,},

    { .pattern = "STATus:PRESet", .callback = SCPI_StatusPreset,},
    
    { .pattern = "TEXTfunction?", .callback = text_function,},

    { .pattern = "TEST:TREEA?", .callback = test_treeA,},
    { .pattern = "TEST:TREEB?", .callback = test_treeB,},

    SCPI_CMD_LIST_END
};


char output_buffer[1024];
size_t output_buffer_pos = 0;

int_fast16_t err_buffer[128];
size_t err_buffer_pos = 0;

static void output_buffer_clear(void) {
    output_buffer[0] = '\0';
    output_buffer_pos = 0;
}

static size_t output_buffer_write(const char * data, size_t len) {
    memcpy(output_buffer + output_buffer_pos, data, len);
    output_buffer_pos += len;
    output_buffer[output_buffer_pos] = '\0';
}

scpi_t scpi_context;
static void error_buffer_clear(void) {
    err_buffer[0] = 0;
    err_buffer_pos = 0;

    SCPI_EventClear(&scpi_context);
    SCPI_ErrorClear(&scpi_context);
}

static void error_buffer_add(int_fast16_t err) {
    err_buffer[err_buffer_pos] = err;
    err_buffer_pos++;
}


static size_t SCPI_Write(scpi_t * context, const char * data, size_t len) {
    (void) context;

    return output_buffer_write(data, len);
}

static scpi_result_t SCPI_Flush(scpi_t * context) {
    return SCPI_RES_OK;
}

static int SCPI_Error(scpi_t * context, int_fast16_t err) {
    (void) context;

    error_buffer_add(err);

    return 0;
}

scpi_reg_val_t srq_val = 0;
static scpi_result_t SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val) {
    if (SCPI_CTRL_SRQ == ctrl) {
        srq_val = val;
    } else {
        fprintf(stderr, "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
    }
    return SCPI_RES_OK;
}

scpi_bool_t RST_executed = FALSE;

static scpi_result_t SCPI_Reset(scpi_t * context) {
    RST_executed = TRUE;
    return SCPI_RES_OK;
}

static scpi_interface_t scpi_interface = {
    .error = SCPI_Error,
    .write = SCPI_Write,
    .control = SCPI_Control,
    .flush = SCPI_Flush,
    .reset = SCPI_Reset,
};

#define SCPI_INPUT_BUFFER_LENGTH 256
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

static scpi_reg_val_t scpi_regs[SCPI_REG_COUNT];


scpi_t scpi_context = {
    .cmdlist = scpi_commands,
    .buffer = {
        .length = SCPI_INPUT_BUFFER_LENGTH,
        .data = scpi_input_buffer,
    },
    .interface = &scpi_interface,
    .registers = scpi_regs,
    .units = scpi_units_def,
    .idn = {"MA", "IN", NULL, "VER"},
};


static int init_suite(void) {
    SCPI_Init(&scpi_context);

    return 0;
}

static int clean_suite(void) {
    return 0;
}

static void testCommandsHandling(void) {
#define TEST_INPUT(data, output) {                              \
    SCPI_Input(&scpi_context, data, strlen(data));              \
    CU_ASSERT_STRING_EQUAL(output, output_buffer);              \
}
    output_buffer_clear();
    error_buffer_clear();

    /* Test single command */
    TEST_INPUT("*IDN?\r\n", "MA,IN,0,VER\r\n");
    output_buffer_clear();

    /* Test multiple commands in input buffer */
    TEST_INPUT("*IDN?\r\n*IDN?\r\n*IDN?\r\n*IDN?\r\n", "MA,IN,0,VER\r\nMA,IN,0,VER\r\nMA,IN,0,VER\r\nMA,IN,0,VER\r\n");
    output_buffer_clear();

    /* Test one command in multiple buffers */
    TEST_INPUT("*IDN?", "");
    TEST_INPUT("\r\n", "MA,IN,0,VER\r\n");
    output_buffer_clear();

    /* Test input "timeout" - input with length == 0 */
    TEST_INPUT("*IDN?", "");
    TEST_INPUT("", "MA,IN,0,VER\r\n");
    output_buffer_clear();
    
    /* Test ctree traversal */
    TEST_INPUT("TEST:TREEA?;TREEB?\r\n", "10\r\n20\r\n");
    output_buffer_clear();

    TEST_INPUT("TEST:TREEA?;:TEXT? \"PARAM1\", \"PARAM2\"\r\n", "10\r\n\"PARAM2\"\r\n");
    output_buffer_clear();

    CU_ASSERT_EQUAL(err_buffer_pos, 0);
    error_buffer_clear();
}

static void testErrorHandling(void) {
    output_buffer_clear();
    error_buffer_clear();

#define TEST_ERROR(data, output, err_num) {                     \
    output_buffer_clear();                                      \
    error_buffer_clear();                                       \
    SCPI_Input(&scpi_context, data, strlen(data));              \
    CU_ASSERT_STRING_EQUAL(output, output_buffer);              \
    CU_ASSERT_EQUAL(err_buffer[0], err_num)                     \
}

    TEST_ERROR("*IDN?\r\n", "MA,IN,0,VER\r\n", 0);
    TEST_ERROR("IDN?\r\n", "", SCPI_ERROR_UNDEFINED_HEADER);
    TEST_ERROR("*ESE\r\n", "", SCPI_ERROR_MISSING_PARAMETER);
    TEST_ERROR("*IDN? 12\r\n", "MA,IN,0,VER\r\n", SCPI_ERROR_PARAMETER_NOT_ALLOWED);
    TEST_ERROR("TEXT? \"PARAM1\", \"PARAM2\"\r\n", "\"PARAM2\"\r\n", 0);

    // TODO: SCPI_ERROR_INVALID_SEPARATOR
    // TODO: SCPI_ERROR_INVALID_SUFFIX
    // TODO: SCPI_ERROR_SUFFIX_NOT_ALLOWED
    // TODO: SCPI_ERROR_EXECUTION_ERROR
    // TODO: SCPI_ERROR_ILLEGAL_PARAMETER_VALUE

    output_buffer_clear();
    error_buffer_clear();
}

static void testIEEE4882(void) {
#define TEST_IEEE4882(data, output) {                           \
    SCPI_Input(&scpi_context, data, strlen(data));              \
    CU_ASSERT_STRING_EQUAL(output, output_buffer);              \
    output_buffer_clear();                                      \
}

    output_buffer_clear();
    error_buffer_clear();

    TEST_IEEE4882("*CLS\r\n", "");
    TEST_IEEE4882("*ESE #H20\r\n", "");
    TEST_IEEE4882("*ESE?\r\n", "32\r\n");
    TEST_IEEE4882("*ESR?\r\n", "0\r\n");
    TEST_IEEE4882("*IDN?\r\n", "MA,IN,0,VER\r\n");
    TEST_IEEE4882("*OPC\r\n", "");
    TEST_IEEE4882("*OPC?\r\n", "1\r\n");

    TEST_IEEE4882("*SRE #HFF\r\n", "");
    TEST_IEEE4882("*SRE?\r\n", "255\r\n");
    TEST_IEEE4882("*STB?\r\n", "0\r\n");
    TEST_IEEE4882("*ESR?\r\n", "1\r\n");
    
    srq_val = 0;
    TEST_IEEE4882("ABCD\r\n", ""); /* "Undefined header" cause command error */
    CU_ASSERT_EQUAL(srq_val, 96); /* value of STB as service request */
    TEST_IEEE4882("*STB?\r\n", "96\r\n"); /* Event status register + Service request */
    TEST_IEEE4882("*ESR?\r\n", "32\r\n"); /* Command error */

    TEST_IEEE4882("*STB?\r\n", "0\r\n");
    TEST_IEEE4882("*ESR?\r\n", "0\r\n");
    
    TEST_IEEE4882("SYST:ERR:NEXT?\r\n", "-113,\"Undefined header\"\r\n");
    TEST_IEEE4882("SYST:ERR:NEXT?\r\n", "0,\"No error\"\r\n");
    
    RST_executed = FALSE;
    TEST_IEEE4882("*RST\r\n", "");
    CU_ASSERT_EQUAL(RST_executed, TRUE);

    TEST_IEEE4882("*TST?\r\n", "0\r\n");
    
    TEST_IEEE4882("*WAI\r\n", "");

    TEST_IEEE4882("SYSTem:VERSion?\r\n", "1999.0\r\n");
}

#define TEST_ParamInt(data, mandatory, expected_value, expected_result, expected_error_code) \
{                                                                                       \
    int32_t value;                                                                      \
    scpi_bool_t result;                                                                 \
    int16_t errCode;                                                                    \
                                                                                        \
    SCPI_CoreCls(&scpi_context);                                                        \
    scpi_context.input_count = 0;                                                       \
    scpi_context.param_list.lex_state.buffer = data;                                    \
    scpi_context.param_list.lex_state.len = strlen(scpi_context.param_list.lex_state.buffer);\
    scpi_context.param_list.lex_state.pos = scpi_context.param_list.lex_state.buffer;   \
    result = SCPI_ParamInt(&scpi_context, &value, mandatory);                           \
                                                                                        \
    errCode = SCPI_ErrorPop(&scpi_context);                                             \
    CU_ASSERT_EQUAL(result, expected_result);                                           \
    if (expected_result) {                                                              \
        CU_ASSERT_EQUAL(value, expected_value);                                         \
    }                                                                                   \
    CU_ASSERT_EQUAL(errCode, expected_error_code);                                      \
}

static void testSCPI_ParamInt(void) {
    TEST_ParamInt("10", TRUE, 10, TRUE, 0);
    TEST_ParamInt("", FALSE, 0, FALSE, 0);
    TEST_ParamInt("10.5", TRUE, 10, TRUE, 0); // TODO: should be FALSE, -104
    TEST_ParamInt("#B101010", TRUE, 42, TRUE, 0);
    TEST_ParamInt("#H101010", TRUE, 1052688, TRUE, 0);
    TEST_ParamInt("#Q10", TRUE, 8, TRUE, 0);

    TEST_ParamInt("", TRUE, 0, FALSE, -109); // missing parameter
    TEST_ParamInt("abcd", TRUE, 0, FALSE, -104); // Data type error
    TEST_ParamInt("10.5V", TRUE, 0, FALSE, -138);
    TEST_ParamInt("10V", TRUE, 0, FALSE, -138);
}

#define TEST_ParamDouble(data, mandatory, expected_value, expected_result, expected_error_code) \
{                                                                                       \
    double value;                                                                       \
    scpi_bool_t result;                                                                 \
    int16_t errCode;                                                                    \
                                                                                        \
    SCPI_CoreCls(&scpi_context);                                                        \
    scpi_context.input_count = 0;                                                       \
    scpi_context.param_list.lex_state.buffer = data;                                    \
    scpi_context.param_list.lex_state.len = strlen(scpi_context.param_list.lex_state.buffer);\
    scpi_context.param_list.lex_state.pos = scpi_context.param_list.lex_state.buffer;   \
    result = SCPI_ParamDouble(&scpi_context, &value, mandatory);                        \
                                                                                        \
    errCode = SCPI_ErrorPop(&scpi_context);                                             \
    CU_ASSERT_EQUAL(result, expected_result);                                           \
    if (expected_result) {                                                              \
        CU_ASSERT_DOUBLE_EQUAL(value, expected_value, 0.000001);                        \
    }                                                                                   \
    CU_ASSERT_EQUAL(errCode, expected_error_code);                                      \
}

static void testSCPI_ParamDouble(void) {
    TEST_ParamDouble("10", TRUE, 10, TRUE, 0);
    TEST_ParamDouble("", FALSE, 0, FALSE, 0);
    TEST_ParamDouble("10.5", TRUE, 10.5, TRUE, 0);
    TEST_ParamDouble("#B101010", TRUE, 42, TRUE, 0);
    TEST_ParamDouble("#H101010", TRUE, 1052688, TRUE, 0);
    TEST_ParamDouble("#Q10", TRUE, 8, TRUE, 0);

    TEST_ParamDouble("", TRUE, 0, FALSE, -109); // missing parameter
    TEST_ParamDouble("abcd", TRUE, 0, FALSE, -104); // Data type error
    TEST_ParamDouble("10.5V", TRUE, 0, FALSE, -138);
    TEST_ParamDouble("10V", TRUE, 0, FALSE, -138);
}

#define TEST_ParamCharacters(data, mandatory, expected_value, expected_result, expected_error_code) \
{                                                                                       \
    const char * value;                                                                 \
    size_t value_len;                                                                   \
    scpi_bool_t result;                                                                 \
    int16_t errCode;                                                                    \
                                                                                        \
    SCPI_CoreCls(&scpi_context);                                                        \
    scpi_context.input_count = 0;                                                       \
    scpi_context.param_list.lex_state.buffer = data;                                    \
    scpi_context.param_list.lex_state.len = strlen(scpi_context.param_list.lex_state.buffer);\
    scpi_context.param_list.lex_state.pos = scpi_context.param_list.lex_state.buffer;   \
    result = SCPI_ParamCharacters(&scpi_context, &value, &value_len, mandatory);        \
    /*printf("%.*s\r\n",  (int)value_len, value);*/                                     \
    errCode = SCPI_ErrorPop(&scpi_context);                                             \
    CU_ASSERT_EQUAL(result, expected_result);                                           \
    if (expected_result) {                                                              \
        CU_ASSERT_NSTRING_EQUAL(value, expected_value, value_len);                      \
    }                                                                                   \
    CU_ASSERT_EQUAL(errCode, expected_error_code);                                      \
}

static void testSCPI_ParamCharacters(void) {
    TEST_ParamCharacters("10", TRUE, "10", TRUE, 0);
    TEST_ParamCharacters(" ABCD", TRUE, "ABCD", TRUE, 0); // TokProgramMnemonic
    TEST_ParamCharacters("\"ABCD\"", TRUE, "ABCD", TRUE, 0); // TokDoubleQuoteProgramData
    TEST_ParamCharacters("\'ABCD\'", TRUE, "ABCD", TRUE, 0); // TokSingleQuoteProgramData
    TEST_ParamCharacters("#204ABCD", TRUE, "ABCD", TRUE, 0); // TokArbitraryBlockProgramData
    TEST_ParamCharacters("#210ABCD", TRUE, "", FALSE, -151); // invalid Block Data
}


int main() {
    CU_pSuite pSuite = NULL;

    /* Initialize the CUnit test registry */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Add a suite to the registry */
    pSuite = CU_add_suite("Parser", init_suite, clean_suite);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Add the tests to the suite */
    if ((NULL == CU_add_test(pSuite, "SCPI_ParamInt", testSCPI_ParamInt))
            || (NULL == CU_add_test(pSuite, "SCPI_ParamDouble", testSCPI_ParamDouble))
            || (NULL == CU_add_test(pSuite, "SCPI_ParamCharacters", testSCPI_ParamCharacters))
	    || (NULL == CU_add_test(pSuite, "Commands handling", testCommandsHandling))
	    || (NULL == CU_add_test(pSuite, "Error handling", testErrorHandling))
	    || (NULL == CU_add_test(pSuite, "IEEE 488.2 Mandatory commands", testIEEE4882))
            ) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    /* Run all tests using the CUnit Basic interface */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

