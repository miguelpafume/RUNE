Projeto:
- Desenvolvimento do zero de um emulador para um console de videogame de 16 bits, programado em C++ com foco na utilização da API gráfica moderna Vulkan. 

Objetivo:
- Imitar o comportamento do hardware original com ferramentas modernas.
- Estudo prático de engenharia reversa

Legalidade:
- Fazer a emulação dos componentes (CPU, PPU, DSP, etc.) que fazem parte do dispositivo é completamente legal.
    - O que o tornaria ilegal seria a utilização do código original que faz parte do sistema.
- Sobre as ROMs, ou os jogos em si, é ilegal a distribuição e download digital da cópia do jogo, pois elas estão protegidas por direitos autorais dos fabricantes.
    - Isso não é um grande problema, pois existem ROMs feitas para testes e jogos distribuídos de graça pela internet, feitas por fãs desenvolvedores.
- Qualquer patente de hardware que a Nintendo tinha expirou, elas duram normalmente por 20 anos. Ou seja, tudo que não seja os nomes “SNES”, “Nintendo”, “Super Nintendo” e afins, que estão relacionados à emulação do sistema, é livre para uso.

Ferramentas:
- C++
    - Usando a versão C++20 com o compilador GCC
    - Publicado em 12/2020
- Vulkan
    - API gráfica multiplataforma de alto desempenho que permite interação direta com a GPU
    - Publicado em 02/2016, ainda é mantido com sua última atualização estável em 04/2026
- GLFW
    - API para criação de janelas, contextos e interfaces, input, manuseio de eventos, etc.
    - Publicado em 02/2002, ainda é mantido com sua última atualização estável em 02/2024
- ImGUI
    - Biblioteca para interface
    - Publicado em 08/2014, ainda é mantido com sua última atualização estável em 04/2026
- Outros Projetos:
    - Bsnes: um emulador com foco em precisão que utiliza Vulkan, não recebe atualizações desde 2020.
    - Snes9X: emulador popular com suporte ao nível “frontend” para Vulkan (display para a tela, shaders, etc.)