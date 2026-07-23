# Conversor Boost Utilizando Raspberry Pi Pico W: Uma Solução de Potência Versátil para Sistemas Embarcados

## Visão Geral do Projeto

Este projeto, desenvolvido com a placa BitDogLab e o microcontrolador Raspberry Pi Pico W, tem como objetivo principal a concepção e integração de um robusto circuito de potência baseado em um conversor DC-DC step-up, conhecido como conversor **boost**. A motivação para tal reside na crescente demanda por flexibilidade na alimentação de componentes em sistemas embarcados modernos. Para uma análise detalhada, o documento completo está [disponível aqui](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0), onde se explora como microcontroladores operam tipicamente com baixas tensões (como os 3.3V do Raspberry Pi Pico W), enquanto muitos atuadores essenciais – como motores, LEDs de alta potência e outros dispositivos – requerem tensões significativamente maiores para operar de forma eficiente.

Este projeto tem como objetivo desenvolver e integrar um circuito de potência baseado em um conversor DC-DC step-up (boost) com a placa BitDogLab, utilizando o microcontrolador Raspberry Pi Pico W. Enquanto microcontroladores operam tipicamente com baixas tensões (como 3.3V do microcontrolador em uso), muitos atuadores, como motores e LEDs de alta potência, exigem tensões significativamente maiores.

Neste cenário, o conversor boost surge como uma solução indispensável, permitindo otimizar o uso da energia disponível e expandir as capacidades intrínsecas do sistema. O controle preciso do conversor é realizado diretamente pelo Raspberry Pi Pico W, aproveitando sua capacidade de geração de sinais PWM (Pulse Width Modulation). Para garantir a segurança e a integridade do microcontrolador, esse sinal PWM é isolado galvanicamente através de um optoacoplador (o modelo U3 - 4N25 é utilizado), e subsequentemente amplificado por um gate driver discreto (composto pelos transistores Q2, Q3, Q4 e componentes associados) antes de acionar um MOSFET de potência (D3 - IRF540N), que é o componente chave para a conversão.

## A Crucial Importância do Circuito de Potência (Conversor Boost)

O circuito de potência, com o conversor boost em seu cerne, representa a fundação e a capacidade de "musculatura" deste projeto. Sua importância é inquestionável em qualquer sistema embarcado que exija acionamento de dispositivos com requisitos de tensão mais elevados do que o microcontrolador pode fornecer diretamente. A flexibilidade que o boost converter oferece é uma virada de jogo, permitindo que dispositivos de baixa potência, como o Pico W, controlem cargas de média a alta potência. Mais detalhes sobre essa funcionalidade podem ser encontrados no [documento completo do projeto](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0).

A relevância do conversor é destacada no texto principal:
> "Neste contexto, conversores DC-DC step-up, ou 'boost', tornam-se indispensáveis para otimizar o uso da energia disponível e expandir as capacidades do sistema."

Além de elevar a tensão, a arquitetura do circuito de potência implementada neste projeto foi cuidadosamente projetada para garantir robustez e segurança. A utilização de **planos de terra separados** (GND1 para a parte de controle da BitDogLab e GND2 para a parte de potência do circuito boost) é uma estratégia fundamental para mitigar a propagação de ruídos elétricos, gerados pelas rápidas comutações do MOSFET, para a sensível lógica digital do microcontrolador. Essa medida previne interferências e assegura a estabilidade do sistema.

O robusto gate driver discreto (com transistores como BC337, BD140, BD139) é vital para o acionamento eficiente do MOSFET. Ele é capaz de fornecer a corrente necessária para carregar e descarregar rapidamente a capacitância de porta do MOSFET, garantindo uma comutação ágil e minimizando perdas. O dimensionamento deste conversor em particular foi otimizado para uma tensão de entrada de 5V e uma saída regulada de 12V, com capacidade para fornecer até 1A de corrente à carga, resultando em uma potência de saída de 12W. Essa capacidade de amplificação de tensão, combinada com o isolamento e a proteção, torna o circuito de potência um pilar para diversas aplicações. Para informações adicionais sobre o dimensionamento, consulte o [documento do projeto](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0).

## Funcionalidades e Componentes Chave

O desenvolvimento do projeto seguiu uma abordagem dividida em etapas bem definidas para garantir a sua funcionalidade e robustez, conforme descrito em detalhes no [documento completo](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0):

1.  **Design e Dimensionamento do Circuito Conversor Boost:** Esta fase envolveu a definição precisa dos requisitos de entrada e saída (tensão, corrente, potência), a seleção criteriosa e o dimensionamento de todos os componentes críticos. Isso inclui os componentes do gate driver discreto (transistores, resistores, diodo de proteção), o MOSFET de potência (série IRF), o indutor de potência (com núcleo de ferrite) e os capacitores de entrada/saída, além do diodo do conversor Boost. O esquemático detalhado foi elaborado no KiCad, uma ferramenta essencial para a validação das conexões e valores dos componentes.

2.  **Implementação do Firmware e Controle do Pico W:** Esta etapa focou na configuração e programação do módulo PWM do microcontrolador RP2040. O objetivo foi gerar pulsos de controle com a frequência e duty cycle exatos necessários para o funcionamento otimizado do conversor boost, utilizando a GPIO 20 do Raspberry Pi Pico W. Foi fundamental estabelecer uma interface robusta entre o Pico W e o circuito de potência, assegurando a correta utilização dos GPIOs configurados para PWM, a compatibilidade dos níveis lógicos e a operação segura do optoacoplador e do gate driver.

**Componentes de Hardware Utilizados:**

Uma lista completa e detalhada dos componentes de hardware, juntamente com suas funções específicas e diagramas, pode ser consultada no [documento do projeto](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0). Alguns dos principais incluem:

*   **Placa BitDogLab:** Atua como a base de desenvolvimento principal para o microcontrolador.
*   **Raspberry Pi Pico W (RP2040):** O coração do controle, um microcontrolador dual-core com módulos PWM de alta resolução.
*   **Optoacoplador 4N25 (U3):** Garante o isolamento galvânico crítico do sinal PWM de controle, protegendo o microcontrolador.
*   **Transistores de Sinal e Potência (Q2 - BC337, Q3 - BD140, Q4 - BD139):** Compõem o gate driver discreto, responsáveis pela amplificação e acionamento eficiente do MOSFET.
*   **MOSFET de Potência (D3 - IRF540N):** O dispositivo chave de comutação do conversor boost, adequado para as tensões e correntes envolvidas.
*   **Indutor de Potência (L2):** Um elemento fundamental para o armazenamento e transferência de energia no conversor.
*   **Diodo de Recuperação Rápida ou Schottky (incorporado no D3 e D1 no driver):** Essencial para a retificação da saída do boost.
*   **Resistores diversos (R2, R3, R4, R5, R6, R7, R8, R10):** Utilizados para polarização, limitação de corrente e funções de pull-up/down.
*   **Capacitores Eletrolíticos (C1 - 10μF, C2 - 10μF):** Capacitores de baixa ESR (Equivalent Series Resistance) para filtragem de entrada e saída, e possível C3 para desacoplamento.
*   **Diodos de Proteção (D1, D3 - corpo diode):** Oferecem proteção contra transientes de tensão e durante o chaveamento.
*   **Terminais e Conectores (J1, J2, J3, J4, J5):** Para as conexões de potência, carga, PWM e outras necessidades.
*   **LED Indicador (D2) com Resistor Limitador (R7):** Fornece feedback visual do status, indicando que há tensão na entrada.

**Software e Linguagens Utilizadas:**

A implementação do software foi crucial para o controle do sistema, e os detalhes técnicos podem ser explorados no [material de apoio](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0). As principais ferramentas e linguagens incluem:

*   **Linguagem C:** A linguagem de programação utilizada para o firmware, aproveitando o SDK oficial da Raspberry Pi.
*   **Raspberry Pi Pico C/C++ SDK:** O SDK oficial que oferece suporte aos módulos GPIO, ADC, PWM e outros recursos necessários para o controle.
*   **CMake:** O sistema de build empregado para compilar e organizar o projeto C/C++.
*   **KiCad:** Software fundamental para a elaboração e validação do esquemático e do layout da placa de circuito impresso (PCB).

## Implementação de Firmware

A capacidade de controle do Raspberry Pi Pico W é demonstrada na implementação do firmware, que gerencia o sinal PWM para o conversor boost. O código foi configurado para uma operação precisa, e o trecho fundamental para a configuração do PWM é apresentado no [documento do projeto](https://docs.google.com/document/d/1OuxyFDfI4rMv4tx7KyJrpwKmmSDGhWM3hd5obKrCM_8/edit?tab=t.0):

*   **Pino de Saída do PWM:** Definido como `GPIO 20`.
*   **Frequência do PWM:** Estabelecida em `100 kHz`.
*   **Duty Cycle do PWM:** Configurado em `55%`.

Essa configuração é essencial para o chaveamento adequado do MOSFET e, consequentemente, para a elevação da tensão na saída do conversor.

```c
#define PWM_OUTPUT_PIN 20 // Define o pino do PWM

// Configuração do PWM:
gpio_set_function(PWM_OUTPUT_PIN, GPIO_FUNC_PWM);
uint pwm_slice = pwm_gpio_to_slice_num(PWM_OUTPUT_PIN);
pwm_set_wrap(pwm_slice, 1249); // Frequência = 100kHz
pwm_set_chan_level(pwm_slice, pwm_gpio_to_channel(PWM_OUTPUT_PIN), 688); // Duty cycle = 55%
pwm_set_enabled(pwm_slice, true);
